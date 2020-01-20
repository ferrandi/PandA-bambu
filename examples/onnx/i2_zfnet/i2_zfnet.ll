; ModuleID = 'fused_multiply_5'
source_filename = "fused_multiply_5"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%0 = type { i8*, %1, i32, %2, i64*, i64*, i64 }
%1 = type { i32, i32 }
%2 = type { i8, i8, i16 }

@__tvm_main__ = weak local_unnamed_addr constant [17 x i8] c"fused_multiply_5\00", align 1

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_multiply_5(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_multiply_5_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_multiply_5_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = load float, float* %4, align 64, !tbaa !5
  %6 = bitcast i8* %0 to float*
  %broadcast.splatinsert3 = insertelement <4 x float> undef, float %5, i32 0
  %broadcast.splat4 = shufflevector <4 x float> %broadcast.splatinsert3, <4 x float> undef, <4 x i32> zeroinitializer
  %broadcast.splatinsert5 = insertelement <4 x float> undef, float %5, i32 0
  %broadcast.splat6 = shufflevector <4 x float> %broadcast.splatinsert5, <4 x float> undef, <4 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ]
  %7 = getelementptr inbounds float, float* %3, i64 %index
  %8 = bitcast float* %7 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !20
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !20
  %11 = fmul <4 x float> %wide.load, %broadcast.splat4
  %12 = fmul <4 x float> %wide.load2, %broadcast.splat6
  %13 = getelementptr inbounds float, float* %6, i64 %index
  %14 = bitcast float* %13 to <4 x float>*
  store <4 x float> %11, <4 x float>* %14, align 4, !tbaa !23
  %15 = getelementptr inbounds float, float* %13, i64 4
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %12, <4 x float>* %16, align 4, !tbaa !23
  %index.next = add i64 %index, 8
  %17 = icmp eq i64 %index.next, 1000
  br i1 %17, label %for_end, label %vector.body, !llvm.loop !26

for_end:                                          ; preds = %vector.body
  ret void
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
  %3 = alloca [109 x <8 x float>], align 32
  %4 = alloca [1764 x <8 x float>], align 16
  %5 = alloca [149187 x float], align 4
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_begin1.preheader, %entry
  %indvar = phi i64 [ 0, %entry ], [ %indvar.next, %for_begin1.preheader ]
  %6 = mul nuw nsw i64 %indvar, 669
  %7 = mul nuw nsw i64 %indvar, 896
  %scevgep = getelementptr [149187 x float], [149187 x float]* %5, i64 0, i64 %6
  %scevgep27 = bitcast float* %scevgep to i8*
  %scevgep28 = getelementptr i8, i8* %0, i64 %7
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep27, i8* align 4 %scevgep28, i64 892, i1 false)
  %8 = add nuw nsw i64 %6, 223
  %scevgep.1 = getelementptr [149187 x float], [149187 x float]* %5, i64 0, i64 %8
  %scevgep27.1 = bitcast float* %scevgep.1 to i8*
  %9 = add nuw nsw i64 %7, 200704
  %scevgep28.1 = getelementptr i8, i8* %0, i64 %9
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep27.1, i8* align 4 %scevgep28.1, i64 892, i1 false)
  %10 = add nuw nsw i64 %6, 446
  %scevgep.2 = getelementptr [149187 x float], [149187 x float]* %5, i64 0, i64 %10
  %scevgep27.2 = bitcast float* %scevgep.2 to i8*
  %11 = add nuw nsw i64 %7, 401408
  %scevgep28.2 = getelementptr i8, i8* %0, i64 %11
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep27.2, i8* align 4 %scevgep28.2, i64 892, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond30 = icmp eq i64 %indvar.next, 223
  br i1 %exitcond30, label %for_begin7.preheader, label %for_begin1.preheader, !prof !28

for_begin7.preheader:                             ; preds = %for_begin1.preheader
  %12 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %indvars.iv19 = phi i64 [ 0, %for_begin7.preheader ], [ %indvars.iv.next20, %for_end12 ]
  %13 = mul nuw nsw i64 %indvars.iv19, 168
  %14 = trunc i64 %indvars.iv19 to i32
  %15 = urem i32 %14, 7
  %16 = mul nuw nsw i32 %15, 7
  %17 = udiv i32 %14, 7
  %18 = mul nsw i32 %17, 1176
  %19 = add nuw i32 %16, %18
  %20 = zext i32 %19 to i64
  br label %for_begin13.preheader

for_begin16.preheader:                            ; preds = %for_end12
  %21 = bitcast i8* %2 to float*
  br label %for_begin19.preheader

for_begin13.preheader:                            ; preds = %for_begin13.preheader, %for_begin10.preheader
  %indvars.iv16 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next17, %for_begin13.preheader ]
  %22 = mul nuw nsw i64 %indvars.iv16, 24
  %23 = add nuw nsw i64 %22, %13
  %24 = add nuw nsw i64 %indvars.iv16, %20
  %25 = add nuw nsw i64 %24, 147
  %26 = add nuw nsw i64 %24, 294
  %27 = add nuw nsw i64 %24, 441
  %28 = add nuw nsw i64 %24, 588
  %29 = add nuw nsw i64 %24, 735
  %30 = add nuw nsw i64 %24, 882
  %31 = add nuw nsw i64 %24, 1029
  %32 = getelementptr inbounds float, float* %12, i64 %24
  %33 = load float, float* %32, align 4, !tbaa !29
  %34 = insertelement <8 x float> undef, float %33, i32 0
  %35 = getelementptr inbounds float, float* %12, i64 %25
  %36 = load float, float* %35, align 4, !tbaa !29
  %37 = insertelement <8 x float> %34, float %36, i32 1
  %38 = getelementptr inbounds float, float* %12, i64 %26
  %39 = load float, float* %38, align 4, !tbaa !29
  %40 = insertelement <8 x float> %37, float %39, i32 2
  %41 = getelementptr inbounds float, float* %12, i64 %27
  %42 = load float, float* %41, align 4, !tbaa !29
  %43 = insertelement <8 x float> %40, float %42, i32 3
  %44 = getelementptr inbounds float, float* %12, i64 %28
  %45 = load float, float* %44, align 4, !tbaa !29
  %46 = insertelement <8 x float> %43, float %45, i32 4
  %47 = getelementptr inbounds float, float* %12, i64 %29
  %48 = load float, float* %47, align 4, !tbaa !29
  %49 = insertelement <8 x float> %46, float %48, i32 5
  %50 = getelementptr inbounds float, float* %12, i64 %30
  %51 = load float, float* %50, align 4, !tbaa !29
  %52 = insertelement <8 x float> %49, float %51, i32 6
  %53 = getelementptr inbounds float, float* %12, i64 %31
  %54 = load float, float* %53, align 4, !tbaa !29
  %55 = insertelement <8 x float> %52, float %54, i32 7
  %56 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %23
  %57 = bitcast float* %56 to <8 x float>*
  store <8 x float> %55, <8 x float>* %57, align 16, !tbaa !32
  %58 = add nuw nsw i64 %23, 8
  %59 = add nuw nsw i64 %24, 49
  %60 = add nuw nsw i64 %24, 196
  %61 = add nuw nsw i64 %24, 343
  %62 = add nuw nsw i64 %24, 490
  %63 = add nuw nsw i64 %24, 637
  %64 = add nuw nsw i64 %24, 784
  %65 = add nuw nsw i64 %24, 931
  %66 = add nuw nsw i64 %24, 1078
  %67 = getelementptr inbounds float, float* %12, i64 %59
  %68 = load float, float* %67, align 4, !tbaa !29
  %69 = insertelement <8 x float> undef, float %68, i32 0
  %70 = getelementptr inbounds float, float* %12, i64 %60
  %71 = load float, float* %70, align 4, !tbaa !29
  %72 = insertelement <8 x float> %69, float %71, i32 1
  %73 = getelementptr inbounds float, float* %12, i64 %61
  %74 = load float, float* %73, align 4, !tbaa !29
  %75 = insertelement <8 x float> %72, float %74, i32 2
  %76 = getelementptr inbounds float, float* %12, i64 %62
  %77 = load float, float* %76, align 4, !tbaa !29
  %78 = insertelement <8 x float> %75, float %77, i32 3
  %79 = getelementptr inbounds float, float* %12, i64 %63
  %80 = load float, float* %79, align 4, !tbaa !29
  %81 = insertelement <8 x float> %78, float %80, i32 4
  %82 = getelementptr inbounds float, float* %12, i64 %64
  %83 = load float, float* %82, align 4, !tbaa !29
  %84 = insertelement <8 x float> %81, float %83, i32 5
  %85 = getelementptr inbounds float, float* %12, i64 %65
  %86 = load float, float* %85, align 4, !tbaa !29
  %87 = insertelement <8 x float> %84, float %86, i32 6
  %88 = getelementptr inbounds float, float* %12, i64 %66
  %89 = load float, float* %88, align 4, !tbaa !29
  %90 = insertelement <8 x float> %87, float %89, i32 7
  %91 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %58
  %92 = bitcast float* %91 to <8 x float>*
  store <8 x float> %90, <8 x float>* %92, align 16, !tbaa !32
  %93 = add nuw nsw i64 %23, 16
  %94 = add nuw nsw i64 %24, 98
  %95 = add nuw nsw i64 %24, 245
  %96 = add nuw nsw i64 %24, 392
  %97 = add nuw nsw i64 %24, 539
  %98 = add nuw nsw i64 %24, 686
  %99 = add nuw nsw i64 %24, 833
  %100 = add nuw nsw i64 %24, 980
  %101 = add nuw nsw i64 %24, 1127
  %102 = getelementptr inbounds float, float* %12, i64 %94
  %103 = load float, float* %102, align 4, !tbaa !29
  %104 = insertelement <8 x float> undef, float %103, i32 0
  %105 = getelementptr inbounds float, float* %12, i64 %95
  %106 = load float, float* %105, align 4, !tbaa !29
  %107 = insertelement <8 x float> %104, float %106, i32 1
  %108 = getelementptr inbounds float, float* %12, i64 %96
  %109 = load float, float* %108, align 4, !tbaa !29
  %110 = insertelement <8 x float> %107, float %109, i32 2
  %111 = getelementptr inbounds float, float* %12, i64 %97
  %112 = load float, float* %111, align 4, !tbaa !29
  %113 = insertelement <8 x float> %110, float %112, i32 3
  %114 = getelementptr inbounds float, float* %12, i64 %98
  %115 = load float, float* %114, align 4, !tbaa !29
  %116 = insertelement <8 x float> %113, float %115, i32 4
  %117 = getelementptr inbounds float, float* %12, i64 %99
  %118 = load float, float* %117, align 4, !tbaa !29
  %119 = insertelement <8 x float> %116, float %118, i32 5
  %120 = getelementptr inbounds float, float* %12, i64 %100
  %121 = load float, float* %120, align 4, !tbaa !29
  %122 = insertelement <8 x float> %119, float %121, i32 6
  %123 = getelementptr inbounds float, float* %12, i64 %101
  %124 = load float, float* %123, align 4, !tbaa !29
  %125 = insertelement <8 x float> %122, float %124, i32 7
  %126 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %93
  %127 = bitcast float* %126 to <8 x float>*
  store <8 x float> %125, <8 x float>* %127, align 16, !tbaa !32
  %indvars.iv.next17 = add nuw nsw i64 %indvars.iv16, 1
  %exitcond18 = icmp eq i64 %indvars.iv.next17, 7
  br i1 %exitcond18, label %for_end12, label %for_begin13.preheader, !prof !28

for_end12:                                        ; preds = %for_begin13.preheader
  %indvars.iv.next20 = add nuw nsw i64 %indvars.iv19, 1
  %exitcond21 = icmp eq i64 %indvars.iv.next20, 84
  br i1 %exitcond21, label %for_begin16.preheader, label %for_begin10.preheader, !prof !28

for_begin19.preheader:                            ; preds = %for_end33, %for_begin16.preheader
  %128 = phi i32 [ 0, %for_begin16.preheader ], [ %405, %for_end33 ]
  %129 = urem i32 %128, 109
  %130 = mul nuw nsw i32 %129, 1338
  %131 = udiv i32 %128, 109
  %132 = mul nsw i32 %131, 1176
  %133 = zext i32 %132 to i64
  %134 = zext i32 %130 to i64
  br label %for_begin22.preheader

for_end18:                                        ; preds = %for_end33
  ret void

for_begin31.preheader:                            ; preds = %for_end24
  %135 = mul nuw nsw i32 %129, 109
  %136 = mul nsw i32 %131, 95048
  %137 = add nuw nsw i32 %136, %135
  %138 = zext i32 %137 to i64
  br label %for_body32

for_begin22.preheader:                            ; preds = %for_end24, %for_begin19.preheader
  %indvars.iv6 = phi i64 [ 0, %for_begin19.preheader ], [ %indvars.iv.next7, %for_end24 ]
  %139 = shl nuw nsw i64 %indvars.iv6, 1
  %140 = add nuw nsw i64 %139, %134
  br label %for_begin25.preheader

for_begin25.preheader:                            ; preds = %for_begin25.preheader, %for_begin22.preheader
  %indvars.iv = phi i64 [ 0, %for_begin22.preheader ], [ %indvars.iv.next, %for_begin25.preheader ]
  %.05 = phi <8 x float> [ zeroinitializer, %for_begin22.preheader ], [ %373, %for_begin25.preheader ]
  %141 = mul nuw nsw i64 %indvars.iv, 669
  %142 = add nuw nsw i64 %140, %141
  %143 = mul nuw nsw i64 %indvars.iv, 168
  %144 = add nuw nsw i64 %143, %133
  %145 = and i64 %142, 4294967295
  %146 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %145
  %147 = load float, float* %146, align 4, !tbaa !35
  %148 = insertelement <8 x float> undef, float %147, i32 0
  %149 = shufflevector <8 x float> %148, <8 x float> undef, <8 x i32> zeroinitializer
  %150 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %144
  %151 = bitcast float* %150 to <8 x float>*
  %152 = load <8 x float>, <8 x float>* %151, align 16, !tbaa !32
  %153 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %149, <8 x float> %152, <8 x float> %.05)
  %154 = add nuw i64 %142, 223
  %155 = and i64 %154, 4294967295
  %156 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %155
  %157 = load float, float* %156, align 4, !tbaa !35
  %158 = insertelement <8 x float> undef, float %157, i32 0
  %159 = shufflevector <8 x float> %158, <8 x float> undef, <8 x i32> zeroinitializer
  %160 = add nuw nsw i64 %144, 8
  %161 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %160
  %162 = bitcast float* %161 to <8 x float>*
  %163 = load <8 x float>, <8 x float>* %162, align 16, !tbaa !32
  %164 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %159, <8 x float> %163, <8 x float> %153)
  %165 = add nuw i64 %142, 446
  %166 = and i64 %165, 4294967295
  %167 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %166
  %168 = load float, float* %167, align 4, !tbaa !35
  %169 = insertelement <8 x float> undef, float %168, i32 0
  %170 = shufflevector <8 x float> %169, <8 x float> undef, <8 x i32> zeroinitializer
  %171 = add nuw nsw i64 %144, 16
  %172 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %171
  %173 = bitcast float* %172 to <8 x float>*
  %174 = load <8 x float>, <8 x float>* %173, align 16, !tbaa !32
  %175 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %170, <8 x float> %174, <8 x float> %164)
  %176 = add nuw nsw i64 %142, 1
  %177 = add nuw nsw i64 %144, 24
  %178 = and i64 %176, 4294967295
  %179 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %178
  %180 = load float, float* %179, align 4, !tbaa !35
  %181 = insertelement <8 x float> undef, float %180, i32 0
  %182 = shufflevector <8 x float> %181, <8 x float> undef, <8 x i32> zeroinitializer
  %183 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %177
  %184 = bitcast float* %183 to <8 x float>*
  %185 = load <8 x float>, <8 x float>* %184, align 16, !tbaa !32
  %186 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %182, <8 x float> %185, <8 x float> %175)
  %187 = add nuw i64 %142, 224
  %188 = and i64 %187, 4294967295
  %189 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %188
  %190 = load float, float* %189, align 4, !tbaa !35
  %191 = insertelement <8 x float> undef, float %190, i32 0
  %192 = shufflevector <8 x float> %191, <8 x float> undef, <8 x i32> zeroinitializer
  %193 = add nuw nsw i64 %144, 32
  %194 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %193
  %195 = bitcast float* %194 to <8 x float>*
  %196 = load <8 x float>, <8 x float>* %195, align 16, !tbaa !32
  %197 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %192, <8 x float> %196, <8 x float> %186)
  %198 = add nuw i64 %142, 447
  %199 = and i64 %198, 4294967295
  %200 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %199
  %201 = load float, float* %200, align 4, !tbaa !35
  %202 = insertelement <8 x float> undef, float %201, i32 0
  %203 = shufflevector <8 x float> %202, <8 x float> undef, <8 x i32> zeroinitializer
  %204 = add nuw nsw i64 %144, 40
  %205 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %204
  %206 = bitcast float* %205 to <8 x float>*
  %207 = load <8 x float>, <8 x float>* %206, align 16, !tbaa !32
  %208 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %203, <8 x float> %207, <8 x float> %197)
  %209 = add nuw nsw i64 %142, 2
  %210 = add nuw nsw i64 %144, 48
  %211 = and i64 %209, 4294967295
  %212 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %211
  %213 = load float, float* %212, align 4, !tbaa !35
  %214 = insertelement <8 x float> undef, float %213, i32 0
  %215 = shufflevector <8 x float> %214, <8 x float> undef, <8 x i32> zeroinitializer
  %216 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %210
  %217 = bitcast float* %216 to <8 x float>*
  %218 = load <8 x float>, <8 x float>* %217, align 16, !tbaa !32
  %219 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %215, <8 x float> %218, <8 x float> %208)
  %220 = add nuw i64 %142, 225
  %221 = and i64 %220, 4294967295
  %222 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %221
  %223 = load float, float* %222, align 4, !tbaa !35
  %224 = insertelement <8 x float> undef, float %223, i32 0
  %225 = shufflevector <8 x float> %224, <8 x float> undef, <8 x i32> zeroinitializer
  %226 = add nuw nsw i64 %144, 56
  %227 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %226
  %228 = bitcast float* %227 to <8 x float>*
  %229 = load <8 x float>, <8 x float>* %228, align 16, !tbaa !32
  %230 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %225, <8 x float> %229, <8 x float> %219)
  %231 = add nuw i64 %142, 448
  %232 = and i64 %231, 4294967295
  %233 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %232
  %234 = load float, float* %233, align 4, !tbaa !35
  %235 = insertelement <8 x float> undef, float %234, i32 0
  %236 = shufflevector <8 x float> %235, <8 x float> undef, <8 x i32> zeroinitializer
  %237 = add nuw nsw i64 %144, 64
  %238 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %237
  %239 = bitcast float* %238 to <8 x float>*
  %240 = load <8 x float>, <8 x float>* %239, align 16, !tbaa !32
  %241 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %236, <8 x float> %240, <8 x float> %230)
  %242 = add nuw nsw i64 %142, 3
  %243 = add nuw nsw i64 %144, 72
  %244 = and i64 %242, 4294967295
  %245 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %244
  %246 = load float, float* %245, align 4, !tbaa !35
  %247 = insertelement <8 x float> undef, float %246, i32 0
  %248 = shufflevector <8 x float> %247, <8 x float> undef, <8 x i32> zeroinitializer
  %249 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %243
  %250 = bitcast float* %249 to <8 x float>*
  %251 = load <8 x float>, <8 x float>* %250, align 16, !tbaa !32
  %252 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %248, <8 x float> %251, <8 x float> %241)
  %253 = add nuw i64 %142, 226
  %254 = and i64 %253, 4294967295
  %255 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %254
  %256 = load float, float* %255, align 4, !tbaa !35
  %257 = insertelement <8 x float> undef, float %256, i32 0
  %258 = shufflevector <8 x float> %257, <8 x float> undef, <8 x i32> zeroinitializer
  %259 = add nuw nsw i64 %144, 80
  %260 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %259
  %261 = bitcast float* %260 to <8 x float>*
  %262 = load <8 x float>, <8 x float>* %261, align 16, !tbaa !32
  %263 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %258, <8 x float> %262, <8 x float> %252)
  %264 = add nuw i64 %142, 449
  %265 = and i64 %264, 4294967295
  %266 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %265
  %267 = load float, float* %266, align 4, !tbaa !35
  %268 = insertelement <8 x float> undef, float %267, i32 0
  %269 = shufflevector <8 x float> %268, <8 x float> undef, <8 x i32> zeroinitializer
  %270 = add nuw nsw i64 %144, 88
  %271 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %270
  %272 = bitcast float* %271 to <8 x float>*
  %273 = load <8 x float>, <8 x float>* %272, align 16, !tbaa !32
  %274 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %269, <8 x float> %273, <8 x float> %263)
  %275 = add nuw nsw i64 %142, 4
  %276 = add nuw nsw i64 %144, 96
  %277 = and i64 %275, 4294967295
  %278 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %277
  %279 = load float, float* %278, align 4, !tbaa !35
  %280 = insertelement <8 x float> undef, float %279, i32 0
  %281 = shufflevector <8 x float> %280, <8 x float> undef, <8 x i32> zeroinitializer
  %282 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %276
  %283 = bitcast float* %282 to <8 x float>*
  %284 = load <8 x float>, <8 x float>* %283, align 16, !tbaa !32
  %285 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %281, <8 x float> %284, <8 x float> %274)
  %286 = add nuw i64 %142, 227
  %287 = and i64 %286, 4294967295
  %288 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %287
  %289 = load float, float* %288, align 4, !tbaa !35
  %290 = insertelement <8 x float> undef, float %289, i32 0
  %291 = shufflevector <8 x float> %290, <8 x float> undef, <8 x i32> zeroinitializer
  %292 = add nuw nsw i64 %144, 104
  %293 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %292
  %294 = bitcast float* %293 to <8 x float>*
  %295 = load <8 x float>, <8 x float>* %294, align 16, !tbaa !32
  %296 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %291, <8 x float> %295, <8 x float> %285)
  %297 = add nuw i64 %142, 450
  %298 = and i64 %297, 4294967295
  %299 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %298
  %300 = load float, float* %299, align 4, !tbaa !35
  %301 = insertelement <8 x float> undef, float %300, i32 0
  %302 = shufflevector <8 x float> %301, <8 x float> undef, <8 x i32> zeroinitializer
  %303 = add nuw nsw i64 %144, 112
  %304 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %303
  %305 = bitcast float* %304 to <8 x float>*
  %306 = load <8 x float>, <8 x float>* %305, align 16, !tbaa !32
  %307 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %302, <8 x float> %306, <8 x float> %296)
  %308 = add nuw nsw i64 %142, 5
  %309 = add nuw nsw i64 %144, 120
  %310 = and i64 %308, 4294967295
  %311 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %310
  %312 = load float, float* %311, align 4, !tbaa !35
  %313 = insertelement <8 x float> undef, float %312, i32 0
  %314 = shufflevector <8 x float> %313, <8 x float> undef, <8 x i32> zeroinitializer
  %315 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %309
  %316 = bitcast float* %315 to <8 x float>*
  %317 = load <8 x float>, <8 x float>* %316, align 16, !tbaa !32
  %318 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %314, <8 x float> %317, <8 x float> %307)
  %319 = add nuw i64 %142, 228
  %320 = and i64 %319, 4294967295
  %321 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %320
  %322 = load float, float* %321, align 4, !tbaa !35
  %323 = insertelement <8 x float> undef, float %322, i32 0
  %324 = shufflevector <8 x float> %323, <8 x float> undef, <8 x i32> zeroinitializer
  %325 = add nuw nsw i64 %144, 128
  %326 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %325
  %327 = bitcast float* %326 to <8 x float>*
  %328 = load <8 x float>, <8 x float>* %327, align 16, !tbaa !32
  %329 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %324, <8 x float> %328, <8 x float> %318)
  %330 = add nuw i64 %142, 451
  %331 = and i64 %330, 4294967295
  %332 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %331
  %333 = load float, float* %332, align 4, !tbaa !35
  %334 = insertelement <8 x float> undef, float %333, i32 0
  %335 = shufflevector <8 x float> %334, <8 x float> undef, <8 x i32> zeroinitializer
  %336 = add nuw nsw i64 %144, 136
  %337 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %336
  %338 = bitcast float* %337 to <8 x float>*
  %339 = load <8 x float>, <8 x float>* %338, align 16, !tbaa !32
  %340 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %335, <8 x float> %339, <8 x float> %329)
  %341 = add nuw nsw i64 %142, 6
  %342 = add nuw nsw i64 %144, 144
  %343 = and i64 %341, 4294967295
  %344 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %343
  %345 = load float, float* %344, align 4, !tbaa !35
  %346 = insertelement <8 x float> undef, float %345, i32 0
  %347 = shufflevector <8 x float> %346, <8 x float> undef, <8 x i32> zeroinitializer
  %348 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %342
  %349 = bitcast float* %348 to <8 x float>*
  %350 = load <8 x float>, <8 x float>* %349, align 16, !tbaa !32
  %351 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %347, <8 x float> %350, <8 x float> %340)
  %352 = add nuw i64 %142, 229
  %353 = and i64 %352, 4294967295
  %354 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %353
  %355 = load float, float* %354, align 4, !tbaa !35
  %356 = insertelement <8 x float> undef, float %355, i32 0
  %357 = shufflevector <8 x float> %356, <8 x float> undef, <8 x i32> zeroinitializer
  %358 = add nuw nsw i64 %144, 152
  %359 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %358
  %360 = bitcast float* %359 to <8 x float>*
  %361 = load <8 x float>, <8 x float>* %360, align 16, !tbaa !32
  %362 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %357, <8 x float> %361, <8 x float> %351)
  %363 = add nuw i64 %142, 452
  %364 = and i64 %363, 4294967295
  %365 = getelementptr inbounds [149187 x float], [149187 x float]* %5, i64 0, i64 %364
  %366 = load float, float* %365, align 4, !tbaa !35
  %367 = insertelement <8 x float> undef, float %366, i32 0
  %368 = shufflevector <8 x float> %367, <8 x float> undef, <8 x i32> zeroinitializer
  %369 = add nuw nsw i64 %144, 160
  %370 = getelementptr inbounds [1764 x <8 x float>], [1764 x <8 x float>]* %4, i64 0, i64 0, i64 %369
  %371 = bitcast float* %370 to <8 x float>*
  %372 = load <8 x float>, <8 x float>* %371, align 16, !tbaa !32
  %373 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %368, <8 x float> %372, <8 x float> %362)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 7
  br i1 %exitcond, label %for_end24, label %for_begin25.preheader, !prof !28

for_end24:                                        ; preds = %for_begin25.preheader
  %374 = shl nsw i64 %indvars.iv6, 3
  %375 = getelementptr inbounds [109 x <8 x float>], [109 x <8 x float>]* %3, i64 0, i64 0, i64 %374
  %376 = bitcast float* %375 to <8 x float>*
  store <8 x float> %373, <8 x float>* %376, align 32, !tbaa !38
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  %exitcond8 = icmp eq i64 %indvars.iv.next7, 109
  br i1 %exitcond8, label %for_begin31.preheader, label %for_begin22.preheader, !prof !28

for_body32:                                       ; preds = %for_body32, %for_begin31.preheader
  %indvars.iv9 = phi i64 [ 0, %for_begin31.preheader ], [ %indvars.iv.next10, %for_body32 ]
  %377 = add nuw nsw i64 %indvars.iv9, %138
  %378 = add nuw nsw i64 %377, 11881
  %379 = add nuw nsw i64 %377, 23762
  %380 = add nuw nsw i64 %377, 35643
  %381 = add nuw nsw i64 %377, 47524
  %382 = add nuw nsw i64 %377, 59405
  %383 = add nuw nsw i64 %377, 71286
  %384 = add nuw nsw i64 %377, 83167
  %385 = shl nsw i64 %indvars.iv9, 3
  %386 = getelementptr inbounds [109 x <8 x float>], [109 x <8 x float>]* %3, i64 0, i64 0, i64 %385
  %387 = bitcast float* %386 to <8 x float>*
  %388 = load <8 x float>, <8 x float>* %387, align 32, !tbaa !38
  %389 = getelementptr inbounds float, float* %21, i64 %377
  %390 = extractelement <8 x float> %388, i64 0
  store float %390, float* %389, align 4, !tbaa !41
  %391 = getelementptr inbounds float, float* %21, i64 %378
  %392 = extractelement <8 x float> %388, i64 1
  store float %392, float* %391, align 4, !tbaa !41
  %393 = getelementptr inbounds float, float* %21, i64 %379
  %394 = extractelement <8 x float> %388, i64 2
  store float %394, float* %393, align 4, !tbaa !41
  %395 = getelementptr inbounds float, float* %21, i64 %380
  %396 = extractelement <8 x float> %388, i64 3
  store float %396, float* %395, align 4, !tbaa !41
  %397 = getelementptr inbounds float, float* %21, i64 %381
  %398 = extractelement <8 x float> %388, i64 4
  store float %398, float* %397, align 4, !tbaa !41
  %399 = getelementptr inbounds float, float* %21, i64 %382
  %400 = extractelement <8 x float> %388, i64 5
  store float %400, float* %399, align 4, !tbaa !41
  %401 = getelementptr inbounds float, float* %21, i64 %383
  %402 = extractelement <8 x float> %388, i64 6
  store float %402, float* %401, align 4, !tbaa !41
  %403 = getelementptr inbounds float, float* %21, i64 %384
  %404 = extractelement <8 x float> %388, i64 7
  store float %404, float* %403, align 4, !tbaa !41
  %indvars.iv.next10 = add nuw nsw i64 %indvars.iv9, 1
  %exitcond11 = icmp eq i64 %indvars.iv.next10, 109
  br i1 %exitcond11, label %for_end33, label %for_body32, !prof !28

for_end33:                                        ; preds = %for_body32
  %405 = add nuw nsw i32 %128, 1
  %exitcond12 = icmp eq i32 %405, 1308
  br i1 %exitcond12, label %for_end18, label %for_begin19.preheader, !prof !28
}

; Function Attrs: nounwind readnone speculatable
declare <8 x float> @llvm.fmuladd.v8f32(<8 x float>, <8 x float>, <8 x float>) #4

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_multiply_3(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_multiply_3_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_multiply_3_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = load float, float* %4, align 64, !tbaa !44
  %6 = bitcast i8* %0 to float*
  %broadcast.splatinsert3 = insertelement <4 x float> undef, float %5, i32 0
  %broadcast.splat4 = shufflevector <4 x float> %broadcast.splatinsert3, <4 x float> undef, <4 x i32> zeroinitializer
  %broadcast.splatinsert5 = insertelement <4 x float> undef, float %5, i32 0
  %broadcast.splat6 = shufflevector <4 x float> %broadcast.splatinsert5, <4 x float> undef, <4 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ]
  %7 = getelementptr inbounds float, float* %3, i64 %index
  %8 = bitcast float* %7 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !58
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !58
  %11 = fmul <4 x float> %wide.load, %broadcast.splat4
  %12 = fmul <4 x float> %wide.load2, %broadcast.splat6
  %13 = getelementptr inbounds float, float* %6, i64 %index
  %14 = bitcast float* %13 to <4 x float>*
  store <4 x float> %11, <4 x float>* %14, align 4, !tbaa !61
  %15 = getelementptr inbounds float, float* %13, i64 4
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %12, <4 x float>* %16, align 4, !tbaa !61
  %index.next = add i64 %index, 8
  %17 = icmp eq i64 %index.next, 4096
  br i1 %17, label %for_end, label %vector.body, !llvm.loop !64

for_end:                                          ; preds = %vector.body
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_max_pool2d_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_max_pool2d_2_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_max_pool2d_2_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %0 to float*
  %3 = bitcast i8* %1 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv6 = phi i64 [ 0, %entry ], [ %indvars.iv.next7, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv6, 2916
  %5 = mul nuw nsw i64 %indvars.iv6, 11881
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_body5, %for_begin1.preheader
  %indvars.iv3 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next4, %for_body5 ]
  %6 = mul nuw nsw i64 %indvars.iv3, 54
  %7 = add nuw nsw i64 %6, %4
  %8 = mul nuw nsw i64 %indvars.iv3, 218
  %9 = add nuw nsw i64 %8, %5
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %for_begin4.preheader
  %index = phi i64 [ 0, %for_begin4.preheader ], [ %index.next, %vector.body ]
  %10 = add nuw nsw i64 %7, %index
  %11 = getelementptr inbounds float, float* %2, i64 %10
  %12 = shl nuw nsw i64 %index, 1
  %13 = add nuw nsw i64 %9, %12
  %14 = getelementptr inbounds float, float* %3, i64 %13
  %15 = bitcast float* %14 to <8 x float>*
  %wide.vec = load <8 x float>, <8 x float>* %15, align 4, !tbaa !65
  %strided.vec = shufflevector <8 x float> %wide.vec, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %16 = fcmp olt <4 x float> %strided.vec, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %17 = select <4 x i1> %16, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec
  %18 = add nuw nsw i64 %13, 1
  %19 = getelementptr inbounds float, float* %3, i64 %18
  %20 = bitcast float* %19 to <8 x float>*
  %wide.vec9 = load <8 x float>, <8 x float>* %20, align 4, !tbaa !65
  %strided.vec10 = shufflevector <8 x float> %wide.vec9, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec11 = shufflevector <8 x float> %wide.vec9, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %21 = fcmp ogt <4 x float> %17, %strided.vec10
  %22 = select <4 x i1> %21, <4 x float> %17, <4 x float> %strided.vec10
  %23 = fcmp ogt <4 x float> %22, %strided.vec11
  %24 = select <4 x i1> %23, <4 x float> %22, <4 x float> %strided.vec11
  %25 = add nuw nsw i64 %13, 109
  %26 = getelementptr inbounds float, float* %3, i64 %25
  %27 = bitcast float* %26 to <8 x float>*
  %wide.vec12 = load <8 x float>, <8 x float>* %27, align 4, !tbaa !65
  %strided.vec13 = shufflevector <8 x float> %wide.vec12, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %28 = fcmp ogt <4 x float> %24, %strided.vec13
  %29 = select <4 x i1> %28, <4 x float> %24, <4 x float> %strided.vec13
  %30 = add nuw nsw i64 %13, 110
  %31 = getelementptr inbounds float, float* %3, i64 %30
  %32 = bitcast float* %31 to <8 x float>*
  %wide.vec14 = load <8 x float>, <8 x float>* %32, align 4, !tbaa !65
  %strided.vec15 = shufflevector <8 x float> %wide.vec14, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec16 = shufflevector <8 x float> %wide.vec14, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %33 = fcmp ogt <4 x float> %29, %strided.vec15
  %34 = select <4 x i1> %33, <4 x float> %29, <4 x float> %strided.vec15
  %35 = fcmp ogt <4 x float> %34, %strided.vec16
  %36 = select <4 x i1> %35, <4 x float> %34, <4 x float> %strided.vec16
  %37 = add nuw nsw i64 %13, 218
  %38 = getelementptr inbounds float, float* %3, i64 %37
  %39 = bitcast float* %38 to <8 x float>*
  %wide.vec17 = load <8 x float>, <8 x float>* %39, align 4, !tbaa !65
  %strided.vec18 = shufflevector <8 x float> %wide.vec17, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %40 = fcmp ogt <4 x float> %36, %strided.vec18
  %41 = select <4 x i1> %40, <4 x float> %36, <4 x float> %strided.vec18
  %42 = add nuw nsw i64 %13, 219
  %43 = getelementptr inbounds float, float* %3, i64 %42
  %44 = bitcast float* %43 to <8 x float>*
  %wide.vec19 = load <8 x float>, <8 x float>* %44, align 4, !tbaa !65
  %strided.vec20 = shufflevector <8 x float> %wide.vec19, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec21 = shufflevector <8 x float> %wide.vec19, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %45 = fcmp ogt <4 x float> %41, %strided.vec20
  %46 = select <4 x i1> %45, <4 x float> %41, <4 x float> %strided.vec20
  %47 = fcmp ogt <4 x float> %46, %strided.vec21
  %48 = select <4 x i1> %47, <4 x float> %46, <4 x float> %strided.vec21
  %49 = bitcast float* %11 to <4 x float>*
  store <4 x float> %48, <4 x float>* %49, align 4, !tbaa !68
  %index.next = add i64 %index, 4
  %50 = icmp eq i64 %index.next, 52
  br i1 %50, label %for_body5, label %vector.body, !llvm.loop !71

for_end3:                                         ; preds = %for_body5
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  %exitcond8 = icmp eq i64 %indvars.iv.next7, 96
  br i1 %exitcond8, label %for_end, label %for_begin1.preheader, !prof !28

for_body5:                                        ; preds = %vector.body
  %51 = add nuw nsw i64 %7, 52
  %52 = getelementptr inbounds float, float* %2, i64 %51
  %53 = add nuw nsw i64 %9, 104
  %54 = getelementptr inbounds float, float* %3, i64 %53
  %55 = load float, float* %54, align 4, !tbaa !65
  %56 = fcmp olt float %55, 0xC7EFFFFFE0000000
  %57 = select i1 %56, float 0xC7EFFFFFE0000000, float %55
  %58 = add nuw nsw i64 %9, 105
  %59 = getelementptr inbounds float, float* %3, i64 %58
  %60 = load float, float* %59, align 4, !tbaa !65
  %61 = fcmp ogt float %57, %60
  %62 = select i1 %61, float %57, float %60
  %63 = add nuw nsw i64 %9, 106
  %64 = getelementptr inbounds float, float* %3, i64 %63
  %65 = load float, float* %64, align 4, !tbaa !65
  %66 = fcmp ogt float %62, %65
  %67 = select i1 %66, float %62, float %65
  %68 = add nuw nsw i64 %9, 213
  %69 = getelementptr inbounds float, float* %3, i64 %68
  %70 = load float, float* %69, align 4, !tbaa !65
  %71 = fcmp ogt float %67, %70
  %72 = select i1 %71, float %67, float %70
  %73 = add nuw nsw i64 %9, 214
  %74 = getelementptr inbounds float, float* %3, i64 %73
  %75 = load float, float* %74, align 4, !tbaa !65
  %76 = fcmp ogt float %72, %75
  %77 = select i1 %76, float %72, float %75
  %78 = add nuw nsw i64 %9, 215
  %79 = getelementptr inbounds float, float* %3, i64 %78
  %80 = load float, float* %79, align 4, !tbaa !65
  %81 = fcmp ogt float %77, %80
  %82 = select i1 %81, float %77, float %80
  %83 = add nuw nsw i64 %9, 322
  %84 = getelementptr inbounds float, float* %3, i64 %83
  %85 = load float, float* %84, align 4, !tbaa !65
  %86 = fcmp ogt float %82, %85
  %87 = select i1 %86, float %82, float %85
  %88 = add nuw nsw i64 %9, 323
  %89 = getelementptr inbounds float, float* %3, i64 %88
  %90 = load float, float* %89, align 4, !tbaa !65
  %91 = fcmp ogt float %87, %90
  %92 = select i1 %91, float %87, float %90
  %93 = add nuw nsw i64 %9, 324
  %94 = getelementptr inbounds float, float* %3, i64 %93
  %95 = load float, float* %94, align 4, !tbaa !65
  %96 = fcmp ogt float %92, %95
  %97 = select i1 %96, float %92, float %95
  store float %97, float* %52, align 4, !tbaa !68
  %98 = add nuw nsw i64 %7, 53
  %99 = getelementptr inbounds float, float* %2, i64 %98
  %100 = add nuw nsw i64 %9, 106
  %101 = getelementptr inbounds float, float* %3, i64 %100
  %102 = load float, float* %101, align 4, !tbaa !65
  %103 = fcmp olt float %102, 0xC7EFFFFFE0000000
  %104 = select i1 %103, float 0xC7EFFFFFE0000000, float %102
  %105 = add nuw nsw i64 %9, 107
  %106 = getelementptr inbounds float, float* %3, i64 %105
  %107 = load float, float* %106, align 4, !tbaa !65
  %108 = fcmp ogt float %104, %107
  %109 = select i1 %108, float %104, float %107
  %110 = add nuw nsw i64 %9, 108
  %111 = getelementptr inbounds float, float* %3, i64 %110
  %112 = load float, float* %111, align 4, !tbaa !65
  %113 = fcmp ogt float %109, %112
  %114 = select i1 %113, float %109, float %112
  %115 = add nuw nsw i64 %9, 215
  %116 = getelementptr inbounds float, float* %3, i64 %115
  %117 = load float, float* %116, align 4, !tbaa !65
  %118 = fcmp ogt float %114, %117
  %119 = select i1 %118, float %114, float %117
  %120 = add nuw nsw i64 %9, 216
  %121 = getelementptr inbounds float, float* %3, i64 %120
  %122 = load float, float* %121, align 4, !tbaa !65
  %123 = fcmp ogt float %119, %122
  %124 = select i1 %123, float %119, float %122
  %125 = add nuw nsw i64 %9, 217
  %126 = getelementptr inbounds float, float* %3, i64 %125
  %127 = load float, float* %126, align 4, !tbaa !65
  %128 = fcmp ogt float %124, %127
  %129 = select i1 %128, float %124, float %127
  %130 = add nuw nsw i64 %9, 324
  %131 = getelementptr inbounds float, float* %3, i64 %130
  %132 = load float, float* %131, align 4, !tbaa !65
  %133 = fcmp ogt float %129, %132
  %134 = select i1 %133, float %129, float %132
  %135 = add nuw nsw i64 %9, 325
  %136 = getelementptr inbounds float, float* %3, i64 %135
  %137 = load float, float* %136, align 4, !tbaa !65
  %138 = fcmp ogt float %134, %137
  %139 = select i1 %138, float %134, float %137
  %140 = add nuw nsw i64 %9, 326
  %141 = getelementptr inbounds float, float* %3, i64 %140
  %142 = load float, float* %141, align 4, !tbaa !65
  %143 = fcmp ogt float %139, %142
  %144 = select i1 %143, float %139, float %142
  store float %144, float* %99, align 4, !tbaa !68
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 54
  br i1 %exitcond5, label %for_end3, label %for_begin4.preheader, !prof !28
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_lrn_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
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
  tail call fastcc void @fused_nn_lrn_1_compute_(i8* %9, i8* %11)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_lrn_1_compute_(i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #3 {
entry:
  %2 = alloca [1140576 x float], align 16
  %3 = alloca [1188100 x float], align 16
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar37 = phi i64 [ 0, %entry ], [ %indvar.next38, %for_end3 ]
  %4 = phi i32 [ 0, %entry ], [ %13, %for_end3 ]
  %5 = mul nuw nsw i64 %indvar37, 11881
  %6 = mul nuw nsw i64 %indvar37, 47524
  %7 = add nsw i64 %6, -95048
  %.off = add nsw i32 %4, -2
  %8 = icmp ult i32 %.off, 96
  br i1 %8, label %for_begin4.preheader.us, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep43 = getelementptr [1188100 x float], [1188100 x float]* %3, i64 0, i64 %5
  %scevgep4344 = bitcast float* %scevgep43 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 4 %scevgep4344, i8 0, i64 47524, i1 false)
  br label %for_end3

for_begin4.preheader.us:                          ; preds = %for_begin1.preheader, %for_begin4.preheader.us
  %indvar48 = phi i64 [ %indvar.next49, %for_begin4.preheader.us ], [ 0, %for_begin1.preheader ]
  %9 = mul nuw nsw i64 %indvar48, 109
  %10 = add nuw nsw i64 %5, %9
  %scevgep = getelementptr [1188100 x float], [1188100 x float]* %3, i64 0, i64 %10
  %scevgep50 = bitcast float* %scevgep to i8*
  %11 = mul nuw nsw i64 %indvar48, 436
  %12 = add nsw i64 %7, %11
  %scevgep51 = getelementptr i8, i8* %0, i64 %12
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep50, i8* align 4 %scevgep51, i64 436, i1 false)
  %indvar.next49 = add nuw nsw i64 %indvar48, 1
  %exitcond52 = icmp eq i64 %indvar.next49, 109
  br i1 %exitcond52, label %for_end3, label %for_begin4.preheader.us, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader.us, %for_begin4.preheader.preheader
  %13 = add nuw nsw i32 %4, 1
  %indvar.next38 = add nuw nsw i64 %indvar37, 1
  %exitcond53 = icmp eq i64 %indvar.next38, 100
  br i1 %exitcond53, label %for_begin10.preheader.preheader, label %for_begin1.preheader, !prof !28

for_begin10.preheader.preheader:                  ; preds = %for_end3
  %14 = bitcast i8* %0 to float*
  br label %for_begin10.preheader

for_begin10.preheader:                            ; preds = %for_end12, %for_begin10.preheader.preheader
  %indvar = phi i64 [ 0, %for_begin10.preheader.preheader ], [ %indvar.next, %for_end12 ]
  %15 = mul nuw nsw i64 %indvar, 11881
  br label %for_begin13.preheader

for_begin13.preheader:                            ; preds = %for_body14, %for_begin10.preheader
  %indvar29 = phi i64 [ 0, %for_begin10.preheader ], [ %indvar.next30, %for_body14 ]
  %16 = mul nuw nsw i64 %indvar29, 109
  %17 = add nuw nsw i64 %16, %15
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %for_begin13.preheader
  %index = phi i64 [ 0, %for_begin13.preheader ], [ %index.next, %vector.body ]
  %18 = add nuw nsw i64 %17, %index
  %19 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %18
  %20 = getelementptr inbounds [1188100 x float], [1188100 x float]* %3, i64 0, i64 %18
  %21 = bitcast float* %20 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %21, align 4, !tbaa !72
  %22 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load, <4 x float> %wide.load, <4 x float> zeroinitializer)
  %23 = add nuw nsw i64 %18, 11881
  %24 = getelementptr inbounds [1188100 x float], [1188100 x float]* %3, i64 0, i64 %23
  %25 = bitcast float* %24 to <4 x float>*
  %wide.load54 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !72
  %26 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54, <4 x float> %wide.load54, <4 x float> %22)
  %27 = add nuw nsw i64 %18, 23762
  %28 = getelementptr inbounds [1188100 x float], [1188100 x float]* %3, i64 0, i64 %27
  %29 = bitcast float* %28 to <4 x float>*
  %wide.load55 = load <4 x float>, <4 x float>* %29, align 4, !tbaa !72
  %30 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55, <4 x float> %wide.load55, <4 x float> %26)
  %31 = add nuw nsw i64 %18, 35643
  %32 = getelementptr inbounds [1188100 x float], [1188100 x float]* %3, i64 0, i64 %31
  %33 = bitcast float* %32 to <4 x float>*
  %wide.load56 = load <4 x float>, <4 x float>* %33, align 4, !tbaa !72
  %34 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56, <4 x float> %wide.load56, <4 x float> %30)
  %35 = add nuw nsw i64 %18, 47524
  %36 = getelementptr inbounds [1188100 x float], [1188100 x float]* %3, i64 0, i64 %35
  %37 = bitcast float* %36 to <4 x float>*
  %wide.load57 = load <4 x float>, <4 x float>* %37, align 4, !tbaa !72
  %38 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57, <4 x float> %wide.load57, <4 x float> %34)
  %39 = bitcast float* %19 to <4 x float>*
  store <4 x float> %38, <4 x float>* %39, align 4, !tbaa !75
  %index.next = add i64 %index, 4
  %40 = icmp eq i64 %index.next, 108
  br i1 %40, label %for_body14, label %vector.body, !llvm.loop !78

for_end12:                                        ; preds = %for_body14
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond33 = icmp eq i64 %indvar.next, 96
  br i1 %exitcond33, label %for_begin22.preheader, label %for_begin10.preheader, !prof !28

for_body14:                                       ; preds = %vector.body
  %41 = add nuw nsw i64 %17, 108
  %42 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %41
  %43 = getelementptr inbounds [1188100 x float], [1188100 x float]* %3, i64 0, i64 %41
  %44 = load float, float* %43, align 4, !tbaa !72
  %45 = tail call float @llvm.fmuladd.f32(float %44, float %44, float 0.000000e+00)
  %46 = add nuw nsw i64 %17, 11989
  %47 = getelementptr inbounds [1188100 x float], [1188100 x float]* %3, i64 0, i64 %46
  %48 = load float, float* %47, align 4, !tbaa !72
  %49 = tail call float @llvm.fmuladd.f32(float %48, float %48, float %45)
  %50 = add nuw nsw i64 %17, 23870
  %51 = getelementptr inbounds [1188100 x float], [1188100 x float]* %3, i64 0, i64 %50
  %52 = load float, float* %51, align 4, !tbaa !72
  %53 = tail call float @llvm.fmuladd.f32(float %52, float %52, float %49)
  %54 = add nuw nsw i64 %17, 35751
  %55 = getelementptr inbounds [1188100 x float], [1188100 x float]* %3, i64 0, i64 %54
  %56 = load float, float* %55, align 4, !tbaa !72
  %57 = tail call float @llvm.fmuladd.f32(float %56, float %56, float %53)
  %58 = add nuw nsw i64 %17, 47632
  %59 = getelementptr inbounds [1188100 x float], [1188100 x float]* %3, i64 0, i64 %58
  %60 = load float, float* %59, align 4, !tbaa !72
  %61 = tail call float @llvm.fmuladd.f32(float %60, float %60, float %57)
  store float %61, float* %42, align 4, !tbaa !75
  %indvar.next30 = add nuw nsw i64 %indvar29, 1
  %exitcond32 = icmp eq i64 %indvar.next30, 109
  br i1 %exitcond32, label %for_end12, label %for_begin13.preheader, !prof !28

for_begin22.preheader:                            ; preds = %for_end12, %for_end24
  %indvars.iv20 = phi i64 [ %indvars.iv.next21, %for_end24 ], [ 0, %for_end12 ]
  %62 = mul nuw nsw i64 %indvars.iv20, 11881
  br label %for_begin25.preheader

for_begin28.preheader:                            ; preds = %for_end24
  %63 = bitcast i8* %1 to float*
  br label %for_begin31.preheader

for_begin25.preheader:                            ; preds = %for_begin25.preheader, %for_begin22.preheader
  %indvars.iv17 = phi i64 [ 0, %for_begin22.preheader ], [ %indvars.iv.next18, %for_begin25.preheader ]
  %64 = mul nuw nsw i64 %indvars.iv17, 109
  %65 = add nuw nsw i64 %64, %62
  %66 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %65
  %67 = bitcast float* %66 to <4 x float>*
  %wide.load69 = load <4 x float>, <4 x float>* %67, align 4, !tbaa !75
  %68 = fmul <4 x float> %wide.load69, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %69 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %68, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %70 = call <4 x float> @llvm.pow.v4f32(<4 x float> %69, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %71 = bitcast float* %66 to <4 x float>*
  store <4 x float> %70, <4 x float>* %71, align 4, !tbaa !75
  %72 = add nuw nsw i64 %65, 4
  %73 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %72
  %74 = bitcast float* %73 to <4 x float>*
  %wide.load69.1 = load <4 x float>, <4 x float>* %74, align 4, !tbaa !75
  %75 = fmul <4 x float> %wide.load69.1, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %76 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %75, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %77 = call <4 x float> @llvm.pow.v4f32(<4 x float> %76, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %78 = bitcast float* %73 to <4 x float>*
  store <4 x float> %77, <4 x float>* %78, align 4, !tbaa !75
  %79 = add nuw nsw i64 %65, 8
  %80 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %79
  %81 = bitcast float* %80 to <4 x float>*
  %wide.load69.2 = load <4 x float>, <4 x float>* %81, align 4, !tbaa !75
  %82 = fmul <4 x float> %wide.load69.2, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %83 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %82, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %84 = call <4 x float> @llvm.pow.v4f32(<4 x float> %83, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %85 = bitcast float* %80 to <4 x float>*
  store <4 x float> %84, <4 x float>* %85, align 4, !tbaa !75
  %86 = add nuw nsw i64 %65, 12
  %87 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %86
  %88 = bitcast float* %87 to <4 x float>*
  %wide.load69.3 = load <4 x float>, <4 x float>* %88, align 4, !tbaa !75
  %89 = fmul <4 x float> %wide.load69.3, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %90 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %89, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %91 = call <4 x float> @llvm.pow.v4f32(<4 x float> %90, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %92 = bitcast float* %87 to <4 x float>*
  store <4 x float> %91, <4 x float>* %92, align 4, !tbaa !75
  %93 = add nuw nsw i64 %65, 16
  %94 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %93
  %95 = bitcast float* %94 to <4 x float>*
  %wide.load69.4 = load <4 x float>, <4 x float>* %95, align 4, !tbaa !75
  %96 = fmul <4 x float> %wide.load69.4, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %97 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %96, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %98 = call <4 x float> @llvm.pow.v4f32(<4 x float> %97, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %99 = bitcast float* %94 to <4 x float>*
  store <4 x float> %98, <4 x float>* %99, align 4, !tbaa !75
  %100 = add nuw nsw i64 %65, 20
  %101 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %100
  %102 = bitcast float* %101 to <4 x float>*
  %wide.load69.5 = load <4 x float>, <4 x float>* %102, align 4, !tbaa !75
  %103 = fmul <4 x float> %wide.load69.5, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %104 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %103, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %105 = call <4 x float> @llvm.pow.v4f32(<4 x float> %104, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %106 = bitcast float* %101 to <4 x float>*
  store <4 x float> %105, <4 x float>* %106, align 4, !tbaa !75
  %107 = add nuw nsw i64 %65, 24
  %108 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %107
  %109 = bitcast float* %108 to <4 x float>*
  %wide.load69.6 = load <4 x float>, <4 x float>* %109, align 4, !tbaa !75
  %110 = fmul <4 x float> %wide.load69.6, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %111 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %110, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %112 = call <4 x float> @llvm.pow.v4f32(<4 x float> %111, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %113 = bitcast float* %108 to <4 x float>*
  store <4 x float> %112, <4 x float>* %113, align 4, !tbaa !75
  %114 = add nuw nsw i64 %65, 28
  %115 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %114
  %116 = bitcast float* %115 to <4 x float>*
  %wide.load69.7 = load <4 x float>, <4 x float>* %116, align 4, !tbaa !75
  %117 = fmul <4 x float> %wide.load69.7, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %118 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %117, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %119 = call <4 x float> @llvm.pow.v4f32(<4 x float> %118, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %120 = bitcast float* %115 to <4 x float>*
  store <4 x float> %119, <4 x float>* %120, align 4, !tbaa !75
  %121 = add nuw nsw i64 %65, 32
  %122 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %121
  %123 = bitcast float* %122 to <4 x float>*
  %wide.load69.8 = load <4 x float>, <4 x float>* %123, align 4, !tbaa !75
  %124 = fmul <4 x float> %wide.load69.8, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %125 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %124, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %126 = call <4 x float> @llvm.pow.v4f32(<4 x float> %125, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %127 = bitcast float* %122 to <4 x float>*
  store <4 x float> %126, <4 x float>* %127, align 4, !tbaa !75
  %128 = add nuw nsw i64 %65, 36
  %129 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %128
  %130 = bitcast float* %129 to <4 x float>*
  %wide.load69.9 = load <4 x float>, <4 x float>* %130, align 4, !tbaa !75
  %131 = fmul <4 x float> %wide.load69.9, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %132 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %131, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %133 = call <4 x float> @llvm.pow.v4f32(<4 x float> %132, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %134 = bitcast float* %129 to <4 x float>*
  store <4 x float> %133, <4 x float>* %134, align 4, !tbaa !75
  %135 = add nuw nsw i64 %65, 40
  %136 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %135
  %137 = bitcast float* %136 to <4 x float>*
  %wide.load69.10 = load <4 x float>, <4 x float>* %137, align 4, !tbaa !75
  %138 = fmul <4 x float> %wide.load69.10, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %139 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %138, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %140 = call <4 x float> @llvm.pow.v4f32(<4 x float> %139, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %141 = bitcast float* %136 to <4 x float>*
  store <4 x float> %140, <4 x float>* %141, align 4, !tbaa !75
  %142 = add nuw nsw i64 %65, 44
  %143 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %142
  %144 = bitcast float* %143 to <4 x float>*
  %wide.load69.11 = load <4 x float>, <4 x float>* %144, align 4, !tbaa !75
  %145 = fmul <4 x float> %wide.load69.11, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %146 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %145, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %147 = call <4 x float> @llvm.pow.v4f32(<4 x float> %146, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %148 = bitcast float* %143 to <4 x float>*
  store <4 x float> %147, <4 x float>* %148, align 4, !tbaa !75
  %149 = add nuw nsw i64 %65, 48
  %150 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %149
  %151 = bitcast float* %150 to <4 x float>*
  %wide.load69.12 = load <4 x float>, <4 x float>* %151, align 4, !tbaa !75
  %152 = fmul <4 x float> %wide.load69.12, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %153 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %152, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %154 = call <4 x float> @llvm.pow.v4f32(<4 x float> %153, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %155 = bitcast float* %150 to <4 x float>*
  store <4 x float> %154, <4 x float>* %155, align 4, !tbaa !75
  %156 = add nuw nsw i64 %65, 52
  %157 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %156
  %158 = bitcast float* %157 to <4 x float>*
  %wide.load69.13 = load <4 x float>, <4 x float>* %158, align 4, !tbaa !75
  %159 = fmul <4 x float> %wide.load69.13, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %160 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %159, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %161 = call <4 x float> @llvm.pow.v4f32(<4 x float> %160, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %162 = bitcast float* %157 to <4 x float>*
  store <4 x float> %161, <4 x float>* %162, align 4, !tbaa !75
  %163 = add nuw nsw i64 %65, 56
  %164 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %163
  %165 = bitcast float* %164 to <4 x float>*
  %wide.load69.14 = load <4 x float>, <4 x float>* %165, align 4, !tbaa !75
  %166 = fmul <4 x float> %wide.load69.14, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %167 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %166, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %168 = call <4 x float> @llvm.pow.v4f32(<4 x float> %167, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %169 = bitcast float* %164 to <4 x float>*
  store <4 x float> %168, <4 x float>* %169, align 4, !tbaa !75
  %170 = add nuw nsw i64 %65, 60
  %171 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %170
  %172 = bitcast float* %171 to <4 x float>*
  %wide.load69.15 = load <4 x float>, <4 x float>* %172, align 4, !tbaa !75
  %173 = fmul <4 x float> %wide.load69.15, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %174 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %173, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %175 = call <4 x float> @llvm.pow.v4f32(<4 x float> %174, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %176 = bitcast float* %171 to <4 x float>*
  store <4 x float> %175, <4 x float>* %176, align 4, !tbaa !75
  %177 = add nuw nsw i64 %65, 64
  %178 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %177
  %179 = bitcast float* %178 to <4 x float>*
  %wide.load69.16 = load <4 x float>, <4 x float>* %179, align 4, !tbaa !75
  %180 = fmul <4 x float> %wide.load69.16, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %181 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %180, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %182 = call <4 x float> @llvm.pow.v4f32(<4 x float> %181, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %183 = bitcast float* %178 to <4 x float>*
  store <4 x float> %182, <4 x float>* %183, align 4, !tbaa !75
  %184 = add nuw nsw i64 %65, 68
  %185 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %184
  %186 = bitcast float* %185 to <4 x float>*
  %wide.load69.17 = load <4 x float>, <4 x float>* %186, align 4, !tbaa !75
  %187 = fmul <4 x float> %wide.load69.17, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %188 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %187, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %189 = call <4 x float> @llvm.pow.v4f32(<4 x float> %188, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %190 = bitcast float* %185 to <4 x float>*
  store <4 x float> %189, <4 x float>* %190, align 4, !tbaa !75
  %191 = add nuw nsw i64 %65, 72
  %192 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %191
  %193 = bitcast float* %192 to <4 x float>*
  %wide.load69.18 = load <4 x float>, <4 x float>* %193, align 4, !tbaa !75
  %194 = fmul <4 x float> %wide.load69.18, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %195 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %194, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %196 = call <4 x float> @llvm.pow.v4f32(<4 x float> %195, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %197 = bitcast float* %192 to <4 x float>*
  store <4 x float> %196, <4 x float>* %197, align 4, !tbaa !75
  %198 = add nuw nsw i64 %65, 76
  %199 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %198
  %200 = bitcast float* %199 to <4 x float>*
  %wide.load69.19 = load <4 x float>, <4 x float>* %200, align 4, !tbaa !75
  %201 = fmul <4 x float> %wide.load69.19, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %202 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %201, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %203 = call <4 x float> @llvm.pow.v4f32(<4 x float> %202, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %204 = bitcast float* %199 to <4 x float>*
  store <4 x float> %203, <4 x float>* %204, align 4, !tbaa !75
  %205 = add nuw nsw i64 %65, 80
  %206 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %205
  %207 = bitcast float* %206 to <4 x float>*
  %wide.load69.20 = load <4 x float>, <4 x float>* %207, align 4, !tbaa !75
  %208 = fmul <4 x float> %wide.load69.20, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %209 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %208, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %210 = call <4 x float> @llvm.pow.v4f32(<4 x float> %209, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %211 = bitcast float* %206 to <4 x float>*
  store <4 x float> %210, <4 x float>* %211, align 4, !tbaa !75
  %212 = add nuw nsw i64 %65, 84
  %213 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %212
  %214 = bitcast float* %213 to <4 x float>*
  %wide.load69.21 = load <4 x float>, <4 x float>* %214, align 4, !tbaa !75
  %215 = fmul <4 x float> %wide.load69.21, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %216 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %215, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %217 = call <4 x float> @llvm.pow.v4f32(<4 x float> %216, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %218 = bitcast float* %213 to <4 x float>*
  store <4 x float> %217, <4 x float>* %218, align 4, !tbaa !75
  %219 = add nuw nsw i64 %65, 88
  %220 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %219
  %221 = bitcast float* %220 to <4 x float>*
  %wide.load69.22 = load <4 x float>, <4 x float>* %221, align 4, !tbaa !75
  %222 = fmul <4 x float> %wide.load69.22, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %223 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %222, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %224 = call <4 x float> @llvm.pow.v4f32(<4 x float> %223, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %225 = bitcast float* %220 to <4 x float>*
  store <4 x float> %224, <4 x float>* %225, align 4, !tbaa !75
  %226 = add nuw nsw i64 %65, 92
  %227 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %226
  %228 = bitcast float* %227 to <4 x float>*
  %wide.load69.23 = load <4 x float>, <4 x float>* %228, align 4, !tbaa !75
  %229 = fmul <4 x float> %wide.load69.23, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %230 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %229, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %231 = call <4 x float> @llvm.pow.v4f32(<4 x float> %230, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %232 = bitcast float* %227 to <4 x float>*
  store <4 x float> %231, <4 x float>* %232, align 4, !tbaa !75
  %233 = add nuw nsw i64 %65, 96
  %234 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %233
  %235 = bitcast float* %234 to <4 x float>*
  %wide.load69.24 = load <4 x float>, <4 x float>* %235, align 4, !tbaa !75
  %236 = fmul <4 x float> %wide.load69.24, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %237 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %236, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %238 = call <4 x float> @llvm.pow.v4f32(<4 x float> %237, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %239 = bitcast float* %234 to <4 x float>*
  store <4 x float> %238, <4 x float>* %239, align 4, !tbaa !75
  %240 = add nuw nsw i64 %65, 100
  %241 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %240
  %242 = bitcast float* %241 to <4 x float>*
  %wide.load69.25 = load <4 x float>, <4 x float>* %242, align 4, !tbaa !75
  %243 = fmul <4 x float> %wide.load69.25, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %244 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %243, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %245 = call <4 x float> @llvm.pow.v4f32(<4 x float> %244, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %246 = bitcast float* %241 to <4 x float>*
  store <4 x float> %245, <4 x float>* %246, align 4, !tbaa !75
  %247 = add nuw nsw i64 %65, 104
  %248 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %247
  %249 = bitcast float* %248 to <4 x float>*
  %wide.load69.26 = load <4 x float>, <4 x float>* %249, align 4, !tbaa !75
  %250 = fmul <4 x float> %wide.load69.26, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %251 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %250, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %252 = call <4 x float> @llvm.pow.v4f32(<4 x float> %251, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %253 = bitcast float* %248 to <4 x float>*
  store <4 x float> %252, <4 x float>* %253, align 4, !tbaa !75
  %254 = add nuw nsw i64 %65, 108
  %255 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %254
  %256 = load float, float* %255, align 4, !tbaa !75
  %257 = fmul float %256, 0x3F40624DE0000000
  %258 = tail call float @llvm.fmuladd.f32(float %257, float 0x3FC99999A0000000, float 2.000000e+00)
  %259 = tail call float @llvm.pow.f32(float %258, float 7.500000e-01)
  store float %259, float* %255, align 4, !tbaa !75
  %indvars.iv.next18 = add nuw nsw i64 %indvars.iv17, 1
  %exitcond19 = icmp eq i64 %indvars.iv.next18, 109
  br i1 %exitcond19, label %for_end24, label %for_begin25.preheader, !prof !28

for_end24:                                        ; preds = %for_begin25.preheader
  %indvars.iv.next21 = add nuw nsw i64 %indvars.iv20, 1
  %exitcond22 = icmp eq i64 %indvars.iv.next21, 96
  br i1 %exitcond22, label %for_begin28.preheader, label %for_begin22.preheader, !prof !28

for_begin31.preheader:                            ; preds = %for_end33, %for_begin28.preheader
  %indvars.iv11 = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next12, %for_end33 ]
  %260 = mul nuw nsw i64 %indvars.iv11, 11881
  br label %for_begin34.preheader

for_end30:                                        ; preds = %for_end33
  ret void

for_begin34.preheader:                            ; preds = %for_begin34.preheader, %for_begin31.preheader
  %indvars.iv8 = phi i64 [ 0, %for_begin31.preheader ], [ %indvars.iv.next9, %for_begin34.preheader ]
  %261 = mul nuw nsw i64 %indvars.iv8, 109
  %262 = add nuw nsw i64 %261, %260
  %263 = getelementptr inbounds float, float* %14, i64 %262
  %264 = bitcast float* %263 to <4 x float>*
  %wide.load81 = load <4 x float>, <4 x float>* %264, align 4, !tbaa !79
  %265 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %262
  %266 = bitcast float* %265 to <4 x float>*
  %wide.load82 = load <4 x float>, <4 x float>* %266, align 4, !tbaa !75
  %267 = fdiv <4 x float> %wide.load81, %wide.load82
  %268 = getelementptr inbounds float, float* %63, i64 %262
  %269 = bitcast float* %268 to <4 x float>*
  store <4 x float> %267, <4 x float>* %269, align 4, !tbaa !82
  %270 = add nuw nsw i64 %262, 4
  %271 = getelementptr inbounds float, float* %14, i64 %270
  %272 = bitcast float* %271 to <4 x float>*
  %wide.load81.1 = load <4 x float>, <4 x float>* %272, align 4, !tbaa !79
  %273 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %270
  %274 = bitcast float* %273 to <4 x float>*
  %wide.load82.1 = load <4 x float>, <4 x float>* %274, align 4, !tbaa !75
  %275 = fdiv <4 x float> %wide.load81.1, %wide.load82.1
  %276 = getelementptr inbounds float, float* %63, i64 %270
  %277 = bitcast float* %276 to <4 x float>*
  store <4 x float> %275, <4 x float>* %277, align 4, !tbaa !82
  %278 = add nuw nsw i64 %262, 8
  %279 = getelementptr inbounds float, float* %14, i64 %278
  %280 = bitcast float* %279 to <4 x float>*
  %wide.load81.2 = load <4 x float>, <4 x float>* %280, align 4, !tbaa !79
  %281 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %278
  %282 = bitcast float* %281 to <4 x float>*
  %wide.load82.2 = load <4 x float>, <4 x float>* %282, align 4, !tbaa !75
  %283 = fdiv <4 x float> %wide.load81.2, %wide.load82.2
  %284 = getelementptr inbounds float, float* %63, i64 %278
  %285 = bitcast float* %284 to <4 x float>*
  store <4 x float> %283, <4 x float>* %285, align 4, !tbaa !82
  %286 = add nuw nsw i64 %262, 12
  %287 = getelementptr inbounds float, float* %14, i64 %286
  %288 = bitcast float* %287 to <4 x float>*
  %wide.load81.3 = load <4 x float>, <4 x float>* %288, align 4, !tbaa !79
  %289 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %286
  %290 = bitcast float* %289 to <4 x float>*
  %wide.load82.3 = load <4 x float>, <4 x float>* %290, align 4, !tbaa !75
  %291 = fdiv <4 x float> %wide.load81.3, %wide.load82.3
  %292 = getelementptr inbounds float, float* %63, i64 %286
  %293 = bitcast float* %292 to <4 x float>*
  store <4 x float> %291, <4 x float>* %293, align 4, !tbaa !82
  %294 = add nuw nsw i64 %262, 16
  %295 = getelementptr inbounds float, float* %14, i64 %294
  %296 = bitcast float* %295 to <4 x float>*
  %wide.load81.4 = load <4 x float>, <4 x float>* %296, align 4, !tbaa !79
  %297 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %294
  %298 = bitcast float* %297 to <4 x float>*
  %wide.load82.4 = load <4 x float>, <4 x float>* %298, align 4, !tbaa !75
  %299 = fdiv <4 x float> %wide.load81.4, %wide.load82.4
  %300 = getelementptr inbounds float, float* %63, i64 %294
  %301 = bitcast float* %300 to <4 x float>*
  store <4 x float> %299, <4 x float>* %301, align 4, !tbaa !82
  %302 = add nuw nsw i64 %262, 20
  %303 = getelementptr inbounds float, float* %14, i64 %302
  %304 = bitcast float* %303 to <4 x float>*
  %wide.load81.5 = load <4 x float>, <4 x float>* %304, align 4, !tbaa !79
  %305 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %302
  %306 = bitcast float* %305 to <4 x float>*
  %wide.load82.5 = load <4 x float>, <4 x float>* %306, align 4, !tbaa !75
  %307 = fdiv <4 x float> %wide.load81.5, %wide.load82.5
  %308 = getelementptr inbounds float, float* %63, i64 %302
  %309 = bitcast float* %308 to <4 x float>*
  store <4 x float> %307, <4 x float>* %309, align 4, !tbaa !82
  %310 = add nuw nsw i64 %262, 24
  %311 = getelementptr inbounds float, float* %14, i64 %310
  %312 = bitcast float* %311 to <4 x float>*
  %wide.load81.6 = load <4 x float>, <4 x float>* %312, align 4, !tbaa !79
  %313 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %310
  %314 = bitcast float* %313 to <4 x float>*
  %wide.load82.6 = load <4 x float>, <4 x float>* %314, align 4, !tbaa !75
  %315 = fdiv <4 x float> %wide.load81.6, %wide.load82.6
  %316 = getelementptr inbounds float, float* %63, i64 %310
  %317 = bitcast float* %316 to <4 x float>*
  store <4 x float> %315, <4 x float>* %317, align 4, !tbaa !82
  %318 = add nuw nsw i64 %262, 28
  %319 = getelementptr inbounds float, float* %14, i64 %318
  %320 = bitcast float* %319 to <4 x float>*
  %wide.load81.7 = load <4 x float>, <4 x float>* %320, align 4, !tbaa !79
  %321 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %318
  %322 = bitcast float* %321 to <4 x float>*
  %wide.load82.7 = load <4 x float>, <4 x float>* %322, align 4, !tbaa !75
  %323 = fdiv <4 x float> %wide.load81.7, %wide.load82.7
  %324 = getelementptr inbounds float, float* %63, i64 %318
  %325 = bitcast float* %324 to <4 x float>*
  store <4 x float> %323, <4 x float>* %325, align 4, !tbaa !82
  %326 = add nuw nsw i64 %262, 32
  %327 = getelementptr inbounds float, float* %14, i64 %326
  %328 = bitcast float* %327 to <4 x float>*
  %wide.load81.8 = load <4 x float>, <4 x float>* %328, align 4, !tbaa !79
  %329 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %326
  %330 = bitcast float* %329 to <4 x float>*
  %wide.load82.8 = load <4 x float>, <4 x float>* %330, align 4, !tbaa !75
  %331 = fdiv <4 x float> %wide.load81.8, %wide.load82.8
  %332 = getelementptr inbounds float, float* %63, i64 %326
  %333 = bitcast float* %332 to <4 x float>*
  store <4 x float> %331, <4 x float>* %333, align 4, !tbaa !82
  %334 = add nuw nsw i64 %262, 36
  %335 = getelementptr inbounds float, float* %14, i64 %334
  %336 = bitcast float* %335 to <4 x float>*
  %wide.load81.9 = load <4 x float>, <4 x float>* %336, align 4, !tbaa !79
  %337 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %334
  %338 = bitcast float* %337 to <4 x float>*
  %wide.load82.9 = load <4 x float>, <4 x float>* %338, align 4, !tbaa !75
  %339 = fdiv <4 x float> %wide.load81.9, %wide.load82.9
  %340 = getelementptr inbounds float, float* %63, i64 %334
  %341 = bitcast float* %340 to <4 x float>*
  store <4 x float> %339, <4 x float>* %341, align 4, !tbaa !82
  %342 = add nuw nsw i64 %262, 40
  %343 = getelementptr inbounds float, float* %14, i64 %342
  %344 = bitcast float* %343 to <4 x float>*
  %wide.load81.10 = load <4 x float>, <4 x float>* %344, align 4, !tbaa !79
  %345 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %342
  %346 = bitcast float* %345 to <4 x float>*
  %wide.load82.10 = load <4 x float>, <4 x float>* %346, align 4, !tbaa !75
  %347 = fdiv <4 x float> %wide.load81.10, %wide.load82.10
  %348 = getelementptr inbounds float, float* %63, i64 %342
  %349 = bitcast float* %348 to <4 x float>*
  store <4 x float> %347, <4 x float>* %349, align 4, !tbaa !82
  %350 = add nuw nsw i64 %262, 44
  %351 = getelementptr inbounds float, float* %14, i64 %350
  %352 = bitcast float* %351 to <4 x float>*
  %wide.load81.11 = load <4 x float>, <4 x float>* %352, align 4, !tbaa !79
  %353 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %350
  %354 = bitcast float* %353 to <4 x float>*
  %wide.load82.11 = load <4 x float>, <4 x float>* %354, align 4, !tbaa !75
  %355 = fdiv <4 x float> %wide.load81.11, %wide.load82.11
  %356 = getelementptr inbounds float, float* %63, i64 %350
  %357 = bitcast float* %356 to <4 x float>*
  store <4 x float> %355, <4 x float>* %357, align 4, !tbaa !82
  %358 = add nuw nsw i64 %262, 48
  %359 = getelementptr inbounds float, float* %14, i64 %358
  %360 = bitcast float* %359 to <4 x float>*
  %wide.load81.12 = load <4 x float>, <4 x float>* %360, align 4, !tbaa !79
  %361 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %358
  %362 = bitcast float* %361 to <4 x float>*
  %wide.load82.12 = load <4 x float>, <4 x float>* %362, align 4, !tbaa !75
  %363 = fdiv <4 x float> %wide.load81.12, %wide.load82.12
  %364 = getelementptr inbounds float, float* %63, i64 %358
  %365 = bitcast float* %364 to <4 x float>*
  store <4 x float> %363, <4 x float>* %365, align 4, !tbaa !82
  %366 = add nuw nsw i64 %262, 52
  %367 = getelementptr inbounds float, float* %14, i64 %366
  %368 = bitcast float* %367 to <4 x float>*
  %wide.load81.13 = load <4 x float>, <4 x float>* %368, align 4, !tbaa !79
  %369 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %366
  %370 = bitcast float* %369 to <4 x float>*
  %wide.load82.13 = load <4 x float>, <4 x float>* %370, align 4, !tbaa !75
  %371 = fdiv <4 x float> %wide.load81.13, %wide.load82.13
  %372 = getelementptr inbounds float, float* %63, i64 %366
  %373 = bitcast float* %372 to <4 x float>*
  store <4 x float> %371, <4 x float>* %373, align 4, !tbaa !82
  %374 = add nuw nsw i64 %262, 56
  %375 = getelementptr inbounds float, float* %14, i64 %374
  %376 = bitcast float* %375 to <4 x float>*
  %wide.load81.14 = load <4 x float>, <4 x float>* %376, align 4, !tbaa !79
  %377 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %374
  %378 = bitcast float* %377 to <4 x float>*
  %wide.load82.14 = load <4 x float>, <4 x float>* %378, align 4, !tbaa !75
  %379 = fdiv <4 x float> %wide.load81.14, %wide.load82.14
  %380 = getelementptr inbounds float, float* %63, i64 %374
  %381 = bitcast float* %380 to <4 x float>*
  store <4 x float> %379, <4 x float>* %381, align 4, !tbaa !82
  %382 = add nuw nsw i64 %262, 60
  %383 = getelementptr inbounds float, float* %14, i64 %382
  %384 = bitcast float* %383 to <4 x float>*
  %wide.load81.15 = load <4 x float>, <4 x float>* %384, align 4, !tbaa !79
  %385 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %382
  %386 = bitcast float* %385 to <4 x float>*
  %wide.load82.15 = load <4 x float>, <4 x float>* %386, align 4, !tbaa !75
  %387 = fdiv <4 x float> %wide.load81.15, %wide.load82.15
  %388 = getelementptr inbounds float, float* %63, i64 %382
  %389 = bitcast float* %388 to <4 x float>*
  store <4 x float> %387, <4 x float>* %389, align 4, !tbaa !82
  %390 = add nuw nsw i64 %262, 64
  %391 = getelementptr inbounds float, float* %14, i64 %390
  %392 = bitcast float* %391 to <4 x float>*
  %wide.load81.16 = load <4 x float>, <4 x float>* %392, align 4, !tbaa !79
  %393 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %390
  %394 = bitcast float* %393 to <4 x float>*
  %wide.load82.16 = load <4 x float>, <4 x float>* %394, align 4, !tbaa !75
  %395 = fdiv <4 x float> %wide.load81.16, %wide.load82.16
  %396 = getelementptr inbounds float, float* %63, i64 %390
  %397 = bitcast float* %396 to <4 x float>*
  store <4 x float> %395, <4 x float>* %397, align 4, !tbaa !82
  %398 = add nuw nsw i64 %262, 68
  %399 = getelementptr inbounds float, float* %14, i64 %398
  %400 = bitcast float* %399 to <4 x float>*
  %wide.load81.17 = load <4 x float>, <4 x float>* %400, align 4, !tbaa !79
  %401 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %398
  %402 = bitcast float* %401 to <4 x float>*
  %wide.load82.17 = load <4 x float>, <4 x float>* %402, align 4, !tbaa !75
  %403 = fdiv <4 x float> %wide.load81.17, %wide.load82.17
  %404 = getelementptr inbounds float, float* %63, i64 %398
  %405 = bitcast float* %404 to <4 x float>*
  store <4 x float> %403, <4 x float>* %405, align 4, !tbaa !82
  %406 = add nuw nsw i64 %262, 72
  %407 = getelementptr inbounds float, float* %14, i64 %406
  %408 = bitcast float* %407 to <4 x float>*
  %wide.load81.18 = load <4 x float>, <4 x float>* %408, align 4, !tbaa !79
  %409 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %406
  %410 = bitcast float* %409 to <4 x float>*
  %wide.load82.18 = load <4 x float>, <4 x float>* %410, align 4, !tbaa !75
  %411 = fdiv <4 x float> %wide.load81.18, %wide.load82.18
  %412 = getelementptr inbounds float, float* %63, i64 %406
  %413 = bitcast float* %412 to <4 x float>*
  store <4 x float> %411, <4 x float>* %413, align 4, !tbaa !82
  %414 = add nuw nsw i64 %262, 76
  %415 = getelementptr inbounds float, float* %14, i64 %414
  %416 = bitcast float* %415 to <4 x float>*
  %wide.load81.19 = load <4 x float>, <4 x float>* %416, align 4, !tbaa !79
  %417 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %414
  %418 = bitcast float* %417 to <4 x float>*
  %wide.load82.19 = load <4 x float>, <4 x float>* %418, align 4, !tbaa !75
  %419 = fdiv <4 x float> %wide.load81.19, %wide.load82.19
  %420 = getelementptr inbounds float, float* %63, i64 %414
  %421 = bitcast float* %420 to <4 x float>*
  store <4 x float> %419, <4 x float>* %421, align 4, !tbaa !82
  %422 = add nuw nsw i64 %262, 80
  %423 = getelementptr inbounds float, float* %14, i64 %422
  %424 = bitcast float* %423 to <4 x float>*
  %wide.load81.20 = load <4 x float>, <4 x float>* %424, align 4, !tbaa !79
  %425 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %422
  %426 = bitcast float* %425 to <4 x float>*
  %wide.load82.20 = load <4 x float>, <4 x float>* %426, align 4, !tbaa !75
  %427 = fdiv <4 x float> %wide.load81.20, %wide.load82.20
  %428 = getelementptr inbounds float, float* %63, i64 %422
  %429 = bitcast float* %428 to <4 x float>*
  store <4 x float> %427, <4 x float>* %429, align 4, !tbaa !82
  %430 = add nuw nsw i64 %262, 84
  %431 = getelementptr inbounds float, float* %14, i64 %430
  %432 = bitcast float* %431 to <4 x float>*
  %wide.load81.21 = load <4 x float>, <4 x float>* %432, align 4, !tbaa !79
  %433 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %430
  %434 = bitcast float* %433 to <4 x float>*
  %wide.load82.21 = load <4 x float>, <4 x float>* %434, align 4, !tbaa !75
  %435 = fdiv <4 x float> %wide.load81.21, %wide.load82.21
  %436 = getelementptr inbounds float, float* %63, i64 %430
  %437 = bitcast float* %436 to <4 x float>*
  store <4 x float> %435, <4 x float>* %437, align 4, !tbaa !82
  %438 = add nuw nsw i64 %262, 88
  %439 = getelementptr inbounds float, float* %14, i64 %438
  %440 = bitcast float* %439 to <4 x float>*
  %wide.load81.22 = load <4 x float>, <4 x float>* %440, align 4, !tbaa !79
  %441 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %438
  %442 = bitcast float* %441 to <4 x float>*
  %wide.load82.22 = load <4 x float>, <4 x float>* %442, align 4, !tbaa !75
  %443 = fdiv <4 x float> %wide.load81.22, %wide.load82.22
  %444 = getelementptr inbounds float, float* %63, i64 %438
  %445 = bitcast float* %444 to <4 x float>*
  store <4 x float> %443, <4 x float>* %445, align 4, !tbaa !82
  %446 = add nuw nsw i64 %262, 92
  %447 = getelementptr inbounds float, float* %14, i64 %446
  %448 = bitcast float* %447 to <4 x float>*
  %wide.load81.23 = load <4 x float>, <4 x float>* %448, align 4, !tbaa !79
  %449 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %446
  %450 = bitcast float* %449 to <4 x float>*
  %wide.load82.23 = load <4 x float>, <4 x float>* %450, align 4, !tbaa !75
  %451 = fdiv <4 x float> %wide.load81.23, %wide.load82.23
  %452 = getelementptr inbounds float, float* %63, i64 %446
  %453 = bitcast float* %452 to <4 x float>*
  store <4 x float> %451, <4 x float>* %453, align 4, !tbaa !82
  %454 = add nuw nsw i64 %262, 96
  %455 = getelementptr inbounds float, float* %14, i64 %454
  %456 = bitcast float* %455 to <4 x float>*
  %wide.load81.24 = load <4 x float>, <4 x float>* %456, align 4, !tbaa !79
  %457 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %454
  %458 = bitcast float* %457 to <4 x float>*
  %wide.load82.24 = load <4 x float>, <4 x float>* %458, align 4, !tbaa !75
  %459 = fdiv <4 x float> %wide.load81.24, %wide.load82.24
  %460 = getelementptr inbounds float, float* %63, i64 %454
  %461 = bitcast float* %460 to <4 x float>*
  store <4 x float> %459, <4 x float>* %461, align 4, !tbaa !82
  %462 = add nuw nsw i64 %262, 100
  %463 = getelementptr inbounds float, float* %14, i64 %462
  %464 = bitcast float* %463 to <4 x float>*
  %wide.load81.25 = load <4 x float>, <4 x float>* %464, align 4, !tbaa !79
  %465 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %462
  %466 = bitcast float* %465 to <4 x float>*
  %wide.load82.25 = load <4 x float>, <4 x float>* %466, align 4, !tbaa !75
  %467 = fdiv <4 x float> %wide.load81.25, %wide.load82.25
  %468 = getelementptr inbounds float, float* %63, i64 %462
  %469 = bitcast float* %468 to <4 x float>*
  store <4 x float> %467, <4 x float>* %469, align 4, !tbaa !82
  %470 = add nuw nsw i64 %262, 104
  %471 = getelementptr inbounds float, float* %14, i64 %470
  %472 = bitcast float* %471 to <4 x float>*
  %wide.load81.26 = load <4 x float>, <4 x float>* %472, align 4, !tbaa !79
  %473 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %470
  %474 = bitcast float* %473 to <4 x float>*
  %wide.load82.26 = load <4 x float>, <4 x float>* %474, align 4, !tbaa !75
  %475 = fdiv <4 x float> %wide.load81.26, %wide.load82.26
  %476 = getelementptr inbounds float, float* %63, i64 %470
  %477 = bitcast float* %476 to <4 x float>*
  store <4 x float> %475, <4 x float>* %477, align 4, !tbaa !82
  %478 = add nuw nsw i64 %262, 108
  %479 = getelementptr inbounds float, float* %14, i64 %478
  %480 = load float, float* %479, align 4, !tbaa !79
  %481 = getelementptr inbounds [1140576 x float], [1140576 x float]* %2, i64 0, i64 %478
  %482 = load float, float* %481, align 4, !tbaa !75
  %483 = fdiv float %480, %482
  %484 = getelementptr inbounds float, float* %63, i64 %478
  store float %483, float* %484, align 4, !tbaa !82
  %indvars.iv.next9 = add nuw nsw i64 %indvars.iv8, 1
  %exitcond10 = icmp eq i64 %indvars.iv.next9, 109
  br i1 %exitcond10, label %for_end33, label %for_begin34.preheader, !prof !28

for_end33:                                        ; preds = %for_begin34.preheader
  %indvars.iv.next12 = add nuw nsw i64 %indvars.iv11, 1
  %exitcond13 = icmp eq i64 %indvars.iv.next12, 96
  br i1 %exitcond13, label %for_end30, label %for_begin31.preheader, !prof !28
}

; Function Attrs: nounwind readnone speculatable
declare float @llvm.fmuladd.f32(float, float, float) #4

; Function Attrs: nounwind readnone speculatable
declare float @llvm.pow.f32(float, float) #4

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_bias_add_4(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_bias_add_4_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_bias_add_4_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv1 = phi i64 [ 0, %entry ], [ %indvars.iv.next2, %for_end3 ]
  %6 = mul nuw nsw i64 %indvars.iv1, 625
  %7 = getelementptr inbounds float, float* %3, i64 %indvars.iv1
  %8 = load float, float* %7, align 4, !tbaa !85
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
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_begin4.preheader ]
  %21 = mul nuw nsw i64 %indvars.iv, 25
  %22 = add nuw nsw i64 %21, %6
  %23 = getelementptr inbounds float, float* %4, i64 %22
  %24 = getelementptr inbounds float, float* %5, i64 %22
  %25 = bitcast float* %23 to <4 x float>*
  %26 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !88
  %27 = fadd <4 x float> %10, %26
  %28 = bitcast float* %24 to <4 x float>*
  store <4 x float> %27, <4 x float>* %28, align 4, !tbaa !91
  %29 = add nuw nsw i64 %22, 4
  %30 = getelementptr inbounds float, float* %4, i64 %29
  %31 = getelementptr inbounds float, float* %5, i64 %29
  %32 = bitcast float* %30 to <4 x float>*
  %33 = load <4 x float>, <4 x float>* %32, align 4, !tbaa !88
  %34 = fadd <4 x float> %12, %33
  %35 = bitcast float* %31 to <4 x float>*
  store <4 x float> %34, <4 x float>* %35, align 4, !tbaa !91
  %36 = add nuw nsw i64 %22, 8
  %37 = getelementptr inbounds float, float* %4, i64 %36
  %38 = getelementptr inbounds float, float* %5, i64 %36
  %39 = bitcast float* %37 to <4 x float>*
  %40 = load <4 x float>, <4 x float>* %39, align 4, !tbaa !88
  %41 = fadd <4 x float> %14, %40
  %42 = bitcast float* %38 to <4 x float>*
  store <4 x float> %41, <4 x float>* %42, align 4, !tbaa !91
  %43 = add nuw nsw i64 %22, 12
  %44 = getelementptr inbounds float, float* %4, i64 %43
  %45 = getelementptr inbounds float, float* %5, i64 %43
  %46 = bitcast float* %44 to <4 x float>*
  %47 = load <4 x float>, <4 x float>* %46, align 4, !tbaa !88
  %48 = fadd <4 x float> %16, %47
  %49 = bitcast float* %45 to <4 x float>*
  store <4 x float> %48, <4 x float>* %49, align 4, !tbaa !91
  %50 = add nuw nsw i64 %22, 16
  %51 = getelementptr inbounds float, float* %4, i64 %50
  %52 = getelementptr inbounds float, float* %5, i64 %50
  %53 = bitcast float* %51 to <4 x float>*
  %54 = load <4 x float>, <4 x float>* %53, align 4, !tbaa !88
  %55 = fadd <4 x float> %18, %54
  %56 = bitcast float* %52 to <4 x float>*
  store <4 x float> %55, <4 x float>* %56, align 4, !tbaa !91
  %57 = add nuw nsw i64 %22, 20
  %58 = getelementptr inbounds float, float* %4, i64 %57
  %59 = getelementptr inbounds float, float* %5, i64 %57
  %60 = bitcast float* %58 to <4 x float>*
  %61 = load <4 x float>, <4 x float>* %60, align 4, !tbaa !88
  %62 = fadd <4 x float> %20, %61
  %63 = bitcast float* %59 to <4 x float>*
  store <4 x float> %62, <4 x float>* %63, align 4, !tbaa !91
  %64 = add nuw nsw i64 %22, 24
  %65 = getelementptr inbounds float, float* %4, i64 %64
  %66 = load float, float* %65, align 4, !tbaa !88
  %67 = fadd float %8, %66
  %68 = getelementptr inbounds float, float* %5, i64 %64
  store float %67, float* %68, align 4, !tbaa !91
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 25
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 256
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !28
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_relu_3(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_relu_3_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_relu_3_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %1 to float*
  %3 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv1 = phi i64 [ 0, %entry ], [ %indvars.iv.next2, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv1, 625
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_begin4.preheader ]
  %5 = mul nuw nsw i64 %indvars.iv, 25
  %6 = add nuw nsw i64 %5, %4
  %7 = getelementptr inbounds float, float* %2, i64 %6
  %8 = getelementptr inbounds float, float* %3, i64 %6
  %9 = bitcast float* %7 to <4 x float>*
  %10 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !94
  %11 = fcmp ogt <4 x float> %10, zeroinitializer
  %12 = select <4 x i1> %11, <4 x float> %10, <4 x float> zeroinitializer
  %13 = bitcast float* %8 to <4 x float>*
  store <4 x float> %12, <4 x float>* %13, align 4, !tbaa !97
  %14 = add nuw nsw i64 %6, 4
  %15 = getelementptr inbounds float, float* %2, i64 %14
  %16 = getelementptr inbounds float, float* %3, i64 %14
  %17 = bitcast float* %15 to <4 x float>*
  %18 = load <4 x float>, <4 x float>* %17, align 4, !tbaa !94
  %19 = fcmp ogt <4 x float> %18, zeroinitializer
  %20 = select <4 x i1> %19, <4 x float> %18, <4 x float> zeroinitializer
  %21 = bitcast float* %16 to <4 x float>*
  store <4 x float> %20, <4 x float>* %21, align 4, !tbaa !97
  %22 = add nuw nsw i64 %6, 8
  %23 = getelementptr inbounds float, float* %2, i64 %22
  %24 = getelementptr inbounds float, float* %3, i64 %22
  %25 = bitcast float* %23 to <4 x float>*
  %26 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !94
  %27 = fcmp ogt <4 x float> %26, zeroinitializer
  %28 = select <4 x i1> %27, <4 x float> %26, <4 x float> zeroinitializer
  %29 = bitcast float* %24 to <4 x float>*
  store <4 x float> %28, <4 x float>* %29, align 4, !tbaa !97
  %30 = add nuw nsw i64 %6, 12
  %31 = getelementptr inbounds float, float* %2, i64 %30
  %32 = getelementptr inbounds float, float* %3, i64 %30
  %33 = bitcast float* %31 to <4 x float>*
  %34 = load <4 x float>, <4 x float>* %33, align 4, !tbaa !94
  %35 = fcmp ogt <4 x float> %34, zeroinitializer
  %36 = select <4 x i1> %35, <4 x float> %34, <4 x float> zeroinitializer
  %37 = bitcast float* %32 to <4 x float>*
  store <4 x float> %36, <4 x float>* %37, align 4, !tbaa !97
  %38 = add nuw nsw i64 %6, 16
  %39 = getelementptr inbounds float, float* %2, i64 %38
  %40 = getelementptr inbounds float, float* %3, i64 %38
  %41 = bitcast float* %39 to <4 x float>*
  %42 = load <4 x float>, <4 x float>* %41, align 4, !tbaa !94
  %43 = fcmp ogt <4 x float> %42, zeroinitializer
  %44 = select <4 x i1> %43, <4 x float> %42, <4 x float> zeroinitializer
  %45 = bitcast float* %40 to <4 x float>*
  store <4 x float> %44, <4 x float>* %45, align 4, !tbaa !97
  %46 = add nuw nsw i64 %6, 20
  %47 = getelementptr inbounds float, float* %2, i64 %46
  %48 = getelementptr inbounds float, float* %3, i64 %46
  %49 = bitcast float* %47 to <4 x float>*
  %50 = load <4 x float>, <4 x float>* %49, align 4, !tbaa !94
  %51 = fcmp ogt <4 x float> %50, zeroinitializer
  %52 = select <4 x i1> %51, <4 x float> %50, <4 x float> zeroinitializer
  %53 = bitcast float* %48 to <4 x float>*
  store <4 x float> %52, <4 x float>* %53, align 4, !tbaa !97
  %54 = add nuw nsw i64 %6, 24
  %55 = getelementptr inbounds float, float* %2, i64 %54
  %56 = load float, float* %55, align 4, !tbaa !94
  %57 = fcmp ogt float %56, 0.000000e+00
  %58 = select i1 %57, float %56, float 0.000000e+00
  %59 = getelementptr inbounds float, float* %3, i64 %54
  store float %58, float* %59, align 4, !tbaa !97
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 25
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 256
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !28
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_lrn(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
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
  tail call fastcc void @fused_nn_lrn_compute_(i8* %9, i8* %11)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_lrn_compute_(i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #3 {
entry:
  %2 = alloca [160000 x float], align 16
  %3 = alloca [162500 x float], align 16
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar34 = phi i64 [ 0, %entry ], [ %indvar.next35, %for_end3 ]
  %4 = phi i32 [ 0, %entry ], [ %57, %for_end3 ]
  %5 = mul nuw nsw i64 %indvar34, 625
  %6 = mul nuw nsw i64 %indvar34, 2500
  %scevgep40 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %5
  %scevgep4041 = bitcast float* %scevgep40 to i8*
  %.off = add nsw i32 %4, -2
  %7 = icmp ult i32 %.off, 256
  br i1 %7, label %for_begin4.preheader.us.preheader, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  call void @llvm.memset.p0i8.i64(i8* align 4 %scevgep4041, i8 0, i64 2500, i1 false)
  br label %for_end3

for_begin4.preheader.us.preheader:                ; preds = %for_begin1.preheader
  %8 = add nsw i64 %6, -5000
  %scevgep48 = getelementptr i8, i8* %0, i64 %8
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep4041, i8* align 4 %scevgep48, i64 100, i1 false)
  %9 = add nuw nsw i64 %5, 25
  %scevgep.1 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %9
  %scevgep47.1 = bitcast float* %scevgep.1 to i8*
  %10 = add nsw i64 %6, -4900
  %scevgep48.1 = getelementptr i8, i8* %0, i64 %10
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.1, i8* align 4 %scevgep48.1, i64 100, i1 false)
  %11 = add nuw nsw i64 %5, 50
  %scevgep.2 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %11
  %scevgep47.2 = bitcast float* %scevgep.2 to i8*
  %12 = add nsw i64 %6, -4800
  %scevgep48.2 = getelementptr i8, i8* %0, i64 %12
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.2, i8* align 4 %scevgep48.2, i64 100, i1 false)
  %13 = add nuw nsw i64 %5, 75
  %scevgep.3 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %13
  %scevgep47.3 = bitcast float* %scevgep.3 to i8*
  %14 = add nsw i64 %6, -4700
  %scevgep48.3 = getelementptr i8, i8* %0, i64 %14
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.3, i8* align 4 %scevgep48.3, i64 100, i1 false)
  %15 = add nuw nsw i64 %5, 100
  %scevgep.4 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %15
  %scevgep47.4 = bitcast float* %scevgep.4 to i8*
  %16 = add nsw i64 %6, -4600
  %scevgep48.4 = getelementptr i8, i8* %0, i64 %16
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.4, i8* align 4 %scevgep48.4, i64 100, i1 false)
  %17 = add nuw nsw i64 %5, 125
  %scevgep.5 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %17
  %scevgep47.5 = bitcast float* %scevgep.5 to i8*
  %18 = add nsw i64 %6, -4500
  %scevgep48.5 = getelementptr i8, i8* %0, i64 %18
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.5, i8* align 4 %scevgep48.5, i64 100, i1 false)
  %19 = add nuw nsw i64 %5, 150
  %scevgep.6 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %19
  %scevgep47.6 = bitcast float* %scevgep.6 to i8*
  %20 = add nsw i64 %6, -4400
  %scevgep48.6 = getelementptr i8, i8* %0, i64 %20
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.6, i8* align 4 %scevgep48.6, i64 100, i1 false)
  %21 = add nuw nsw i64 %5, 175
  %scevgep.7 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %21
  %scevgep47.7 = bitcast float* %scevgep.7 to i8*
  %22 = add nsw i64 %6, -4300
  %scevgep48.7 = getelementptr i8, i8* %0, i64 %22
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.7, i8* align 4 %scevgep48.7, i64 100, i1 false)
  %23 = add nuw nsw i64 %5, 200
  %scevgep.8 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %23
  %scevgep47.8 = bitcast float* %scevgep.8 to i8*
  %24 = add nsw i64 %6, -4200
  %scevgep48.8 = getelementptr i8, i8* %0, i64 %24
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.8, i8* align 4 %scevgep48.8, i64 100, i1 false)
  %25 = add nuw nsw i64 %5, 225
  %scevgep.9 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %25
  %scevgep47.9 = bitcast float* %scevgep.9 to i8*
  %26 = add nsw i64 %6, -4100
  %scevgep48.9 = getelementptr i8, i8* %0, i64 %26
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.9, i8* align 4 %scevgep48.9, i64 100, i1 false)
  %27 = add nuw nsw i64 %5, 250
  %scevgep.10 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %27
  %scevgep47.10 = bitcast float* %scevgep.10 to i8*
  %28 = add nsw i64 %6, -4000
  %scevgep48.10 = getelementptr i8, i8* %0, i64 %28
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.10, i8* align 4 %scevgep48.10, i64 100, i1 false)
  %29 = add nuw nsw i64 %5, 275
  %scevgep.11 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %29
  %scevgep47.11 = bitcast float* %scevgep.11 to i8*
  %30 = add nsw i64 %6, -3900
  %scevgep48.11 = getelementptr i8, i8* %0, i64 %30
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.11, i8* align 4 %scevgep48.11, i64 100, i1 false)
  %31 = add nuw nsw i64 %5, 300
  %scevgep.12 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %31
  %scevgep47.12 = bitcast float* %scevgep.12 to i8*
  %32 = add nsw i64 %6, -3800
  %scevgep48.12 = getelementptr i8, i8* %0, i64 %32
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.12, i8* align 4 %scevgep48.12, i64 100, i1 false)
  %33 = add nuw nsw i64 %5, 325
  %scevgep.13 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %33
  %scevgep47.13 = bitcast float* %scevgep.13 to i8*
  %34 = add nsw i64 %6, -3700
  %scevgep48.13 = getelementptr i8, i8* %0, i64 %34
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.13, i8* align 4 %scevgep48.13, i64 100, i1 false)
  %35 = add nuw nsw i64 %5, 350
  %scevgep.14 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %35
  %scevgep47.14 = bitcast float* %scevgep.14 to i8*
  %36 = add nsw i64 %6, -3600
  %scevgep48.14 = getelementptr i8, i8* %0, i64 %36
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.14, i8* align 4 %scevgep48.14, i64 100, i1 false)
  %37 = add nuw nsw i64 %5, 375
  %scevgep.15 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %37
  %scevgep47.15 = bitcast float* %scevgep.15 to i8*
  %38 = add nsw i64 %6, -3500
  %scevgep48.15 = getelementptr i8, i8* %0, i64 %38
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.15, i8* align 4 %scevgep48.15, i64 100, i1 false)
  %39 = add nuw nsw i64 %5, 400
  %scevgep.16 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %39
  %scevgep47.16 = bitcast float* %scevgep.16 to i8*
  %40 = add nsw i64 %6, -3400
  %scevgep48.16 = getelementptr i8, i8* %0, i64 %40
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.16, i8* align 4 %scevgep48.16, i64 100, i1 false)
  %41 = add nuw nsw i64 %5, 425
  %scevgep.17 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %41
  %scevgep47.17 = bitcast float* %scevgep.17 to i8*
  %42 = add nsw i64 %6, -3300
  %scevgep48.17 = getelementptr i8, i8* %0, i64 %42
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.17, i8* align 4 %scevgep48.17, i64 100, i1 false)
  %43 = add nuw nsw i64 %5, 450
  %scevgep.18 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %43
  %scevgep47.18 = bitcast float* %scevgep.18 to i8*
  %44 = add nsw i64 %6, -3200
  %scevgep48.18 = getelementptr i8, i8* %0, i64 %44
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.18, i8* align 4 %scevgep48.18, i64 100, i1 false)
  %45 = add nuw nsw i64 %5, 475
  %scevgep.19 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %45
  %scevgep47.19 = bitcast float* %scevgep.19 to i8*
  %46 = add nsw i64 %6, -3100
  %scevgep48.19 = getelementptr i8, i8* %0, i64 %46
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.19, i8* align 4 %scevgep48.19, i64 100, i1 false)
  %47 = add nuw nsw i64 %5, 500
  %scevgep.20 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %47
  %scevgep47.20 = bitcast float* %scevgep.20 to i8*
  %48 = add nsw i64 %6, -3000
  %scevgep48.20 = getelementptr i8, i8* %0, i64 %48
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.20, i8* align 4 %scevgep48.20, i64 100, i1 false)
  %49 = add nuw nsw i64 %5, 525
  %scevgep.21 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %49
  %scevgep47.21 = bitcast float* %scevgep.21 to i8*
  %50 = add nsw i64 %6, -2900
  %scevgep48.21 = getelementptr i8, i8* %0, i64 %50
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.21, i8* align 4 %scevgep48.21, i64 100, i1 false)
  %51 = add nuw nsw i64 %5, 550
  %scevgep.22 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %51
  %scevgep47.22 = bitcast float* %scevgep.22 to i8*
  %52 = add nsw i64 %6, -2800
  %scevgep48.22 = getelementptr i8, i8* %0, i64 %52
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.22, i8* align 4 %scevgep48.22, i64 100, i1 false)
  %53 = add nuw nsw i64 %5, 575
  %scevgep.23 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %53
  %scevgep47.23 = bitcast float* %scevgep.23 to i8*
  %54 = add nsw i64 %6, -2700
  %scevgep48.23 = getelementptr i8, i8* %0, i64 %54
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.23, i8* align 4 %scevgep48.23, i64 100, i1 false)
  %55 = add nuw nsw i64 %5, 600
  %scevgep.24 = getelementptr [162500 x float], [162500 x float]* %3, i64 0, i64 %55
  %scevgep47.24 = bitcast float* %scevgep.24 to i8*
  %56 = add nsw i64 %6, -2600
  %scevgep48.24 = getelementptr i8, i8* %0, i64 %56
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep47.24, i8* align 4 %scevgep48.24, i64 100, i1 false)
  br label %for_end3

for_end3:                                         ; preds = %for_begin4.preheader.preheader, %for_begin4.preheader.us.preheader
  %57 = add nuw nsw i32 %4, 1
  %indvar.next35 = add nuw nsw i64 %indvar34, 1
  %exitcond50 = icmp eq i64 %indvar.next35, 260
  br i1 %exitcond50, label %for_begin10.preheader.preheader, label %for_begin1.preheader, !prof !28

for_begin10.preheader.preheader:                  ; preds = %for_end3
  %58 = bitcast i8* %0 to float*
  br label %for_begin10.preheader

for_begin10.preheader:                            ; preds = %for_end12, %for_begin10.preheader.preheader
  %indvar = phi i64 [ 0, %for_begin10.preheader.preheader ], [ %indvar.next, %for_end12 ]
  %59 = mul nuw nsw i64 %indvar, 625
  br label %for_begin13.preheader

for_begin13.preheader:                            ; preds = %for_begin13.preheader, %for_begin10.preheader
  %indvar26 = phi i64 [ 0, %for_begin10.preheader ], [ %indvar.next27, %for_begin13.preheader ]
  %60 = mul nuw nsw i64 %indvar26, 25
  %61 = add nuw nsw i64 %60, %59
  %62 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %61
  %63 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %61
  %64 = bitcast float* %63 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %64, align 4, !tbaa !100
  %65 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load, <4 x float> %wide.load, <4 x float> zeroinitializer)
  %66 = add nuw nsw i64 %61, 625
  %67 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %66
  %68 = bitcast float* %67 to <4 x float>*
  %wide.load51 = load <4 x float>, <4 x float>* %68, align 4, !tbaa !100
  %69 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load51, <4 x float> %wide.load51, <4 x float> %65)
  %70 = add nuw nsw i64 %61, 1250
  %71 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %70
  %72 = bitcast float* %71 to <4 x float>*
  %wide.load52 = load <4 x float>, <4 x float>* %72, align 4, !tbaa !100
  %73 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load52, <4 x float> %wide.load52, <4 x float> %69)
  %74 = add nuw nsw i64 %61, 1875
  %75 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %74
  %76 = bitcast float* %75 to <4 x float>*
  %wide.load53 = load <4 x float>, <4 x float>* %76, align 4, !tbaa !100
  %77 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load53, <4 x float> %wide.load53, <4 x float> %73)
  %78 = add nuw nsw i64 %61, 2500
  %79 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %78
  %80 = bitcast float* %79 to <4 x float>*
  %wide.load54 = load <4 x float>, <4 x float>* %80, align 4, !tbaa !100
  %81 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54, <4 x float> %wide.load54, <4 x float> %77)
  %82 = bitcast float* %62 to <4 x float>*
  store <4 x float> %81, <4 x float>* %82, align 4, !tbaa !103
  %83 = add nuw nsw i64 %61, 4
  %84 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %83
  %85 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %83
  %86 = bitcast float* %85 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %86, align 4, !tbaa !100
  %87 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.1, <4 x float> %wide.load.1, <4 x float> zeroinitializer)
  %88 = add nuw nsw i64 %61, 629
  %89 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %88
  %90 = bitcast float* %89 to <4 x float>*
  %wide.load51.1 = load <4 x float>, <4 x float>* %90, align 4, !tbaa !100
  %91 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load51.1, <4 x float> %wide.load51.1, <4 x float> %87)
  %92 = add nuw nsw i64 %61, 1254
  %93 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %92
  %94 = bitcast float* %93 to <4 x float>*
  %wide.load52.1 = load <4 x float>, <4 x float>* %94, align 4, !tbaa !100
  %95 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load52.1, <4 x float> %wide.load52.1, <4 x float> %91)
  %96 = add nuw nsw i64 %61, 1879
  %97 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %96
  %98 = bitcast float* %97 to <4 x float>*
  %wide.load53.1 = load <4 x float>, <4 x float>* %98, align 4, !tbaa !100
  %99 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load53.1, <4 x float> %wide.load53.1, <4 x float> %95)
  %100 = add nuw nsw i64 %61, 2504
  %101 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %100
  %102 = bitcast float* %101 to <4 x float>*
  %wide.load54.1 = load <4 x float>, <4 x float>* %102, align 4, !tbaa !100
  %103 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.1, <4 x float> %wide.load54.1, <4 x float> %99)
  %104 = bitcast float* %84 to <4 x float>*
  store <4 x float> %103, <4 x float>* %104, align 4, !tbaa !103
  %105 = add nuw nsw i64 %61, 8
  %106 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %105
  %107 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %105
  %108 = bitcast float* %107 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %108, align 4, !tbaa !100
  %109 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.2, <4 x float> %wide.load.2, <4 x float> zeroinitializer)
  %110 = add nuw nsw i64 %61, 633
  %111 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %110
  %112 = bitcast float* %111 to <4 x float>*
  %wide.load51.2 = load <4 x float>, <4 x float>* %112, align 4, !tbaa !100
  %113 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load51.2, <4 x float> %wide.load51.2, <4 x float> %109)
  %114 = add nuw nsw i64 %61, 1258
  %115 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %114
  %116 = bitcast float* %115 to <4 x float>*
  %wide.load52.2 = load <4 x float>, <4 x float>* %116, align 4, !tbaa !100
  %117 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load52.2, <4 x float> %wide.load52.2, <4 x float> %113)
  %118 = add nuw nsw i64 %61, 1883
  %119 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %118
  %120 = bitcast float* %119 to <4 x float>*
  %wide.load53.2 = load <4 x float>, <4 x float>* %120, align 4, !tbaa !100
  %121 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load53.2, <4 x float> %wide.load53.2, <4 x float> %117)
  %122 = add nuw nsw i64 %61, 2508
  %123 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %122
  %124 = bitcast float* %123 to <4 x float>*
  %wide.load54.2 = load <4 x float>, <4 x float>* %124, align 4, !tbaa !100
  %125 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.2, <4 x float> %wide.load54.2, <4 x float> %121)
  %126 = bitcast float* %106 to <4 x float>*
  store <4 x float> %125, <4 x float>* %126, align 4, !tbaa !103
  %127 = add nuw nsw i64 %61, 12
  %128 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %127
  %129 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %127
  %130 = bitcast float* %129 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %130, align 4, !tbaa !100
  %131 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.3, <4 x float> %wide.load.3, <4 x float> zeroinitializer)
  %132 = add nuw nsw i64 %61, 637
  %133 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %132
  %134 = bitcast float* %133 to <4 x float>*
  %wide.load51.3 = load <4 x float>, <4 x float>* %134, align 4, !tbaa !100
  %135 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load51.3, <4 x float> %wide.load51.3, <4 x float> %131)
  %136 = add nuw nsw i64 %61, 1262
  %137 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %136
  %138 = bitcast float* %137 to <4 x float>*
  %wide.load52.3 = load <4 x float>, <4 x float>* %138, align 4, !tbaa !100
  %139 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load52.3, <4 x float> %wide.load52.3, <4 x float> %135)
  %140 = add nuw nsw i64 %61, 1887
  %141 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %140
  %142 = bitcast float* %141 to <4 x float>*
  %wide.load53.3 = load <4 x float>, <4 x float>* %142, align 4, !tbaa !100
  %143 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load53.3, <4 x float> %wide.load53.3, <4 x float> %139)
  %144 = add nuw nsw i64 %61, 2512
  %145 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %144
  %146 = bitcast float* %145 to <4 x float>*
  %wide.load54.3 = load <4 x float>, <4 x float>* %146, align 4, !tbaa !100
  %147 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.3, <4 x float> %wide.load54.3, <4 x float> %143)
  %148 = bitcast float* %128 to <4 x float>*
  store <4 x float> %147, <4 x float>* %148, align 4, !tbaa !103
  %149 = add nuw nsw i64 %61, 16
  %150 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %149
  %151 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %149
  %152 = bitcast float* %151 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %152, align 4, !tbaa !100
  %153 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.4, <4 x float> %wide.load.4, <4 x float> zeroinitializer)
  %154 = add nuw nsw i64 %61, 641
  %155 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %154
  %156 = bitcast float* %155 to <4 x float>*
  %wide.load51.4 = load <4 x float>, <4 x float>* %156, align 4, !tbaa !100
  %157 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load51.4, <4 x float> %wide.load51.4, <4 x float> %153)
  %158 = add nuw nsw i64 %61, 1266
  %159 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %158
  %160 = bitcast float* %159 to <4 x float>*
  %wide.load52.4 = load <4 x float>, <4 x float>* %160, align 4, !tbaa !100
  %161 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load52.4, <4 x float> %wide.load52.4, <4 x float> %157)
  %162 = add nuw nsw i64 %61, 1891
  %163 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %162
  %164 = bitcast float* %163 to <4 x float>*
  %wide.load53.4 = load <4 x float>, <4 x float>* %164, align 4, !tbaa !100
  %165 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load53.4, <4 x float> %wide.load53.4, <4 x float> %161)
  %166 = add nuw nsw i64 %61, 2516
  %167 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %166
  %168 = bitcast float* %167 to <4 x float>*
  %wide.load54.4 = load <4 x float>, <4 x float>* %168, align 4, !tbaa !100
  %169 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.4, <4 x float> %wide.load54.4, <4 x float> %165)
  %170 = bitcast float* %150 to <4 x float>*
  store <4 x float> %169, <4 x float>* %170, align 4, !tbaa !103
  %171 = add nuw nsw i64 %61, 20
  %172 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %171
  %173 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %171
  %174 = bitcast float* %173 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %174, align 4, !tbaa !100
  %175 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.5, <4 x float> %wide.load.5, <4 x float> zeroinitializer)
  %176 = add nuw nsw i64 %61, 645
  %177 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %176
  %178 = bitcast float* %177 to <4 x float>*
  %wide.load51.5 = load <4 x float>, <4 x float>* %178, align 4, !tbaa !100
  %179 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load51.5, <4 x float> %wide.load51.5, <4 x float> %175)
  %180 = add nuw nsw i64 %61, 1270
  %181 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %180
  %182 = bitcast float* %181 to <4 x float>*
  %wide.load52.5 = load <4 x float>, <4 x float>* %182, align 4, !tbaa !100
  %183 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load52.5, <4 x float> %wide.load52.5, <4 x float> %179)
  %184 = add nuw nsw i64 %61, 1895
  %185 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %184
  %186 = bitcast float* %185 to <4 x float>*
  %wide.load53.5 = load <4 x float>, <4 x float>* %186, align 4, !tbaa !100
  %187 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load53.5, <4 x float> %wide.load53.5, <4 x float> %183)
  %188 = add nuw nsw i64 %61, 2520
  %189 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %188
  %190 = bitcast float* %189 to <4 x float>*
  %wide.load54.5 = load <4 x float>, <4 x float>* %190, align 4, !tbaa !100
  %191 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.5, <4 x float> %wide.load54.5, <4 x float> %187)
  %192 = bitcast float* %172 to <4 x float>*
  store <4 x float> %191, <4 x float>* %192, align 4, !tbaa !103
  %193 = add nuw nsw i64 %61, 24
  %194 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %193
  %195 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %193
  %196 = load float, float* %195, align 4, !tbaa !100
  %197 = tail call float @llvm.fmuladd.f32(float %196, float %196, float 0.000000e+00)
  %198 = add nuw nsw i64 %61, 649
  %199 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %198
  %200 = load float, float* %199, align 4, !tbaa !100
  %201 = tail call float @llvm.fmuladd.f32(float %200, float %200, float %197)
  %202 = add nuw nsw i64 %61, 1274
  %203 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %202
  %204 = load float, float* %203, align 4, !tbaa !100
  %205 = tail call float @llvm.fmuladd.f32(float %204, float %204, float %201)
  %206 = add nuw nsw i64 %61, 1899
  %207 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %206
  %208 = load float, float* %207, align 4, !tbaa !100
  %209 = tail call float @llvm.fmuladd.f32(float %208, float %208, float %205)
  %210 = add nuw nsw i64 %61, 2524
  %211 = getelementptr inbounds [162500 x float], [162500 x float]* %3, i64 0, i64 %210
  %212 = load float, float* %211, align 4, !tbaa !100
  %213 = tail call float @llvm.fmuladd.f32(float %212, float %212, float %209)
  store float %213, float* %194, align 4, !tbaa !103
  %indvar.next27 = add nuw nsw i64 %indvar26, 1
  %exitcond29 = icmp eq i64 %indvar.next27, 25
  br i1 %exitcond29, label %for_end12, label %for_begin13.preheader, !prof !28

for_end12:                                        ; preds = %for_begin13.preheader
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond30 = icmp eq i64 %indvar.next, 256
  br i1 %exitcond30, label %for_begin22.preheader, label %for_begin10.preheader, !prof !28

for_begin22.preheader:                            ; preds = %for_end12, %for_end24
  %indvars.iv17 = phi i64 [ %indvars.iv.next18, %for_end24 ], [ 0, %for_end12 ]
  %214 = mul nuw nsw i64 %indvars.iv17, 625
  br label %for_begin25.preheader

for_begin28.preheader:                            ; preds = %for_end24
  %215 = bitcast i8* %1 to float*
  br label %for_begin31.preheader

for_begin25.preheader:                            ; preds = %for_begin25.preheader, %for_begin22.preheader
  %indvars.iv14 = phi i64 [ 0, %for_begin22.preheader ], [ %indvars.iv.next15, %for_begin25.preheader ]
  %216 = mul nuw nsw i64 %indvars.iv14, 25
  %217 = add nuw nsw i64 %216, %214
  %218 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %217
  %219 = bitcast float* %218 to <4 x float>*
  %220 = load <4 x float>, <4 x float>* %219, align 4, !tbaa !103
  %221 = fmul <4 x float> %220, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %222 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %221, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %223 = call <4 x float> @llvm.pow.v4f32(<4 x float> %222, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %224 = bitcast float* %218 to <4 x float>*
  store <4 x float> %223, <4 x float>* %224, align 4, !tbaa !103
  %225 = add nuw nsw i64 %217, 4
  %226 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %225
  %227 = bitcast float* %226 to <4 x float>*
  %228 = load <4 x float>, <4 x float>* %227, align 4, !tbaa !103
  %229 = fmul <4 x float> %228, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %230 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %229, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %231 = call <4 x float> @llvm.pow.v4f32(<4 x float> %230, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %232 = bitcast float* %226 to <4 x float>*
  store <4 x float> %231, <4 x float>* %232, align 4, !tbaa !103
  %233 = add nuw nsw i64 %217, 8
  %234 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %233
  %235 = bitcast float* %234 to <4 x float>*
  %236 = load <4 x float>, <4 x float>* %235, align 4, !tbaa !103
  %237 = fmul <4 x float> %236, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %238 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %237, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %239 = call <4 x float> @llvm.pow.v4f32(<4 x float> %238, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %240 = bitcast float* %234 to <4 x float>*
  store <4 x float> %239, <4 x float>* %240, align 4, !tbaa !103
  %241 = add nuw nsw i64 %217, 12
  %242 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %241
  %243 = bitcast float* %242 to <4 x float>*
  %244 = load <4 x float>, <4 x float>* %243, align 4, !tbaa !103
  %245 = fmul <4 x float> %244, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %246 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %245, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %247 = call <4 x float> @llvm.pow.v4f32(<4 x float> %246, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %248 = bitcast float* %242 to <4 x float>*
  store <4 x float> %247, <4 x float>* %248, align 4, !tbaa !103
  %249 = add nuw nsw i64 %217, 16
  %250 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %249
  %251 = bitcast float* %250 to <4 x float>*
  %252 = load <4 x float>, <4 x float>* %251, align 4, !tbaa !103
  %253 = fmul <4 x float> %252, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %254 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %253, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %255 = call <4 x float> @llvm.pow.v4f32(<4 x float> %254, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %256 = bitcast float* %250 to <4 x float>*
  store <4 x float> %255, <4 x float>* %256, align 4, !tbaa !103
  %257 = add nuw nsw i64 %217, 20
  %258 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %257
  %259 = bitcast float* %258 to <4 x float>*
  %260 = load <4 x float>, <4 x float>* %259, align 4, !tbaa !103
  %261 = fmul <4 x float> %260, <float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000, float 0x3F40624DE0000000>
  %262 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %261, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 2.000000e+00, float 2.000000e+00, float 2.000000e+00, float 2.000000e+00>)
  %263 = call <4 x float> @llvm.pow.v4f32(<4 x float> %262, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %264 = bitcast float* %258 to <4 x float>*
  store <4 x float> %263, <4 x float>* %264, align 4, !tbaa !103
  %265 = add nuw nsw i64 %217, 24
  %266 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %265
  %267 = load float, float* %266, align 4, !tbaa !103
  %268 = fmul float %267, 0x3F40624DE0000000
  %269 = tail call float @llvm.fmuladd.f32(float %268, float 0x3FC99999A0000000, float 2.000000e+00)
  %270 = tail call float @llvm.pow.f32(float %269, float 7.500000e-01)
  store float %270, float* %266, align 4, !tbaa !103
  %indvars.iv.next15 = add nuw nsw i64 %indvars.iv14, 1
  %exitcond16 = icmp eq i64 %indvars.iv.next15, 25
  br i1 %exitcond16, label %for_end24, label %for_begin25.preheader, !prof !28

for_end24:                                        ; preds = %for_begin25.preheader
  %indvars.iv.next18 = add nuw nsw i64 %indvars.iv17, 1
  %exitcond19 = icmp eq i64 %indvars.iv.next18, 256
  br i1 %exitcond19, label %for_begin28.preheader, label %for_begin22.preheader, !prof !28

for_begin31.preheader:                            ; preds = %for_end33, %for_begin28.preheader
  %indvars.iv8 = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next9, %for_end33 ]
  %271 = mul nuw nsw i64 %indvars.iv8, 625
  br label %for_begin34.preheader

for_end30:                                        ; preds = %for_end33
  ret void

for_begin34.preheader:                            ; preds = %for_begin34.preheader, %for_begin31.preheader
  %indvars.iv = phi i64 [ 0, %for_begin31.preheader ], [ %indvars.iv.next, %for_begin34.preheader ]
  %272 = mul nuw nsw i64 %indvars.iv, 25
  %273 = add nuw nsw i64 %272, %271
  %274 = getelementptr inbounds float, float* %58, i64 %273
  %275 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %273
  %276 = getelementptr inbounds float, float* %215, i64 %273
  %277 = bitcast float* %274 to <4 x float>*
  %278 = load <4 x float>, <4 x float>* %277, align 4, !tbaa !106
  %279 = bitcast float* %275 to <4 x float>*
  %280 = load <4 x float>, <4 x float>* %279, align 4, !tbaa !103
  %281 = fdiv <4 x float> %278, %280
  %282 = bitcast float* %276 to <4 x float>*
  store <4 x float> %281, <4 x float>* %282, align 4, !tbaa !109
  %283 = add nuw nsw i64 %273, 4
  %284 = getelementptr inbounds float, float* %58, i64 %283
  %285 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %283
  %286 = getelementptr inbounds float, float* %215, i64 %283
  %287 = bitcast float* %284 to <4 x float>*
  %288 = load <4 x float>, <4 x float>* %287, align 4, !tbaa !106
  %289 = bitcast float* %285 to <4 x float>*
  %290 = load <4 x float>, <4 x float>* %289, align 4, !tbaa !103
  %291 = fdiv <4 x float> %288, %290
  %292 = bitcast float* %286 to <4 x float>*
  store <4 x float> %291, <4 x float>* %292, align 4, !tbaa !109
  %293 = add nuw nsw i64 %273, 8
  %294 = getelementptr inbounds float, float* %58, i64 %293
  %295 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %293
  %296 = getelementptr inbounds float, float* %215, i64 %293
  %297 = bitcast float* %294 to <4 x float>*
  %298 = load <4 x float>, <4 x float>* %297, align 4, !tbaa !106
  %299 = bitcast float* %295 to <4 x float>*
  %300 = load <4 x float>, <4 x float>* %299, align 4, !tbaa !103
  %301 = fdiv <4 x float> %298, %300
  %302 = bitcast float* %296 to <4 x float>*
  store <4 x float> %301, <4 x float>* %302, align 4, !tbaa !109
  %303 = add nuw nsw i64 %273, 12
  %304 = getelementptr inbounds float, float* %58, i64 %303
  %305 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %303
  %306 = getelementptr inbounds float, float* %215, i64 %303
  %307 = bitcast float* %304 to <4 x float>*
  %308 = load <4 x float>, <4 x float>* %307, align 4, !tbaa !106
  %309 = bitcast float* %305 to <4 x float>*
  %310 = load <4 x float>, <4 x float>* %309, align 4, !tbaa !103
  %311 = fdiv <4 x float> %308, %310
  %312 = bitcast float* %306 to <4 x float>*
  store <4 x float> %311, <4 x float>* %312, align 4, !tbaa !109
  %313 = add nuw nsw i64 %273, 16
  %314 = getelementptr inbounds float, float* %58, i64 %313
  %315 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %313
  %316 = getelementptr inbounds float, float* %215, i64 %313
  %317 = bitcast float* %314 to <4 x float>*
  %318 = load <4 x float>, <4 x float>* %317, align 4, !tbaa !106
  %319 = bitcast float* %315 to <4 x float>*
  %320 = load <4 x float>, <4 x float>* %319, align 4, !tbaa !103
  %321 = fdiv <4 x float> %318, %320
  %322 = bitcast float* %316 to <4 x float>*
  store <4 x float> %321, <4 x float>* %322, align 4, !tbaa !109
  %323 = add nuw nsw i64 %273, 20
  %324 = getelementptr inbounds float, float* %58, i64 %323
  %325 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %323
  %326 = getelementptr inbounds float, float* %215, i64 %323
  %327 = bitcast float* %324 to <4 x float>*
  %328 = load <4 x float>, <4 x float>* %327, align 4, !tbaa !106
  %329 = bitcast float* %325 to <4 x float>*
  %330 = load <4 x float>, <4 x float>* %329, align 4, !tbaa !103
  %331 = fdiv <4 x float> %328, %330
  %332 = bitcast float* %326 to <4 x float>*
  store <4 x float> %331, <4 x float>* %332, align 4, !tbaa !109
  %333 = add nuw nsw i64 %273, 24
  %334 = getelementptr inbounds float, float* %58, i64 %333
  %335 = load float, float* %334, align 4, !tbaa !106
  %336 = getelementptr inbounds [160000 x float], [160000 x float]* %2, i64 0, i64 %333
  %337 = load float, float* %336, align 4, !tbaa !103
  %338 = fdiv float %335, %337
  %339 = getelementptr inbounds float, float* %215, i64 %333
  store float %338, float* %339, align 4, !tbaa !109
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 25
  br i1 %exitcond, label %for_end33, label %for_begin34.preheader, !prof !28

for_end33:                                        ; preds = %for_begin34.preheader
  %indvars.iv.next9 = add nuw nsw i64 %indvars.iv8, 1
  %exitcond10 = icmp eq i64 %indvars.iv.next9, 256
  br i1 %exitcond10, label %for_end30, label %for_begin31.preheader, !prof !28
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_bias_add_5(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_bias_add_5_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_bias_add_5_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv4 = phi i64 [ 0, %entry ], [ %indvars.iv.next5, %for_end3 ]
  %6 = mul nuw nsw i64 %indvars.iv4, 11881
  %7 = getelementptr inbounds float, float* %3, i64 %indvars.iv4
  %8 = load float, float* %7, align 4, !tbaa !112
  %broadcast.splatinsert7 = insertelement <4 x float> undef, float %8, i32 0
  %broadcast.splat8 = shufflevector <4 x float> %broadcast.splatinsert7, <4 x float> undef, <4 x i32> zeroinitializer
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv1 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next2, %for_begin4.preheader ]
  %9 = mul nuw nsw i64 %indvars.iv1, 109
  %10 = add nuw nsw i64 %9, %6
  %11 = getelementptr inbounds float, float* %4, i64 %10
  %12 = bitcast float* %11 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %12, align 4, !tbaa !115
  %13 = fadd <4 x float> %broadcast.splat8, %wide.load
  %14 = getelementptr inbounds float, float* %5, i64 %10
  %15 = bitcast float* %14 to <4 x float>*
  store <4 x float> %13, <4 x float>* %15, align 4, !tbaa !118
  %16 = add nuw nsw i64 %10, 4
  %17 = getelementptr inbounds float, float* %4, i64 %16
  %18 = bitcast float* %17 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %18, align 4, !tbaa !115
  %19 = fadd <4 x float> %broadcast.splat8, %wide.load.1
  %20 = getelementptr inbounds float, float* %5, i64 %16
  %21 = bitcast float* %20 to <4 x float>*
  store <4 x float> %19, <4 x float>* %21, align 4, !tbaa !118
  %22 = add nuw nsw i64 %10, 8
  %23 = getelementptr inbounds float, float* %4, i64 %22
  %24 = bitcast float* %23 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %24, align 4, !tbaa !115
  %25 = fadd <4 x float> %broadcast.splat8, %wide.load.2
  %26 = getelementptr inbounds float, float* %5, i64 %22
  %27 = bitcast float* %26 to <4 x float>*
  store <4 x float> %25, <4 x float>* %27, align 4, !tbaa !118
  %28 = add nuw nsw i64 %10, 12
  %29 = getelementptr inbounds float, float* %4, i64 %28
  %30 = bitcast float* %29 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %30, align 4, !tbaa !115
  %31 = fadd <4 x float> %broadcast.splat8, %wide.load.3
  %32 = getelementptr inbounds float, float* %5, i64 %28
  %33 = bitcast float* %32 to <4 x float>*
  store <4 x float> %31, <4 x float>* %33, align 4, !tbaa !118
  %34 = add nuw nsw i64 %10, 16
  %35 = getelementptr inbounds float, float* %4, i64 %34
  %36 = bitcast float* %35 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %36, align 4, !tbaa !115
  %37 = fadd <4 x float> %broadcast.splat8, %wide.load.4
  %38 = getelementptr inbounds float, float* %5, i64 %34
  %39 = bitcast float* %38 to <4 x float>*
  store <4 x float> %37, <4 x float>* %39, align 4, !tbaa !118
  %40 = add nuw nsw i64 %10, 20
  %41 = getelementptr inbounds float, float* %4, i64 %40
  %42 = bitcast float* %41 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %42, align 4, !tbaa !115
  %43 = fadd <4 x float> %broadcast.splat8, %wide.load.5
  %44 = getelementptr inbounds float, float* %5, i64 %40
  %45 = bitcast float* %44 to <4 x float>*
  store <4 x float> %43, <4 x float>* %45, align 4, !tbaa !118
  %46 = add nuw nsw i64 %10, 24
  %47 = getelementptr inbounds float, float* %4, i64 %46
  %48 = bitcast float* %47 to <4 x float>*
  %wide.load.6 = load <4 x float>, <4 x float>* %48, align 4, !tbaa !115
  %49 = fadd <4 x float> %broadcast.splat8, %wide.load.6
  %50 = getelementptr inbounds float, float* %5, i64 %46
  %51 = bitcast float* %50 to <4 x float>*
  store <4 x float> %49, <4 x float>* %51, align 4, !tbaa !118
  %52 = add nuw nsw i64 %10, 28
  %53 = getelementptr inbounds float, float* %4, i64 %52
  %54 = bitcast float* %53 to <4 x float>*
  %wide.load.7 = load <4 x float>, <4 x float>* %54, align 4, !tbaa !115
  %55 = fadd <4 x float> %broadcast.splat8, %wide.load.7
  %56 = getelementptr inbounds float, float* %5, i64 %52
  %57 = bitcast float* %56 to <4 x float>*
  store <4 x float> %55, <4 x float>* %57, align 4, !tbaa !118
  %58 = add nuw nsw i64 %10, 32
  %59 = getelementptr inbounds float, float* %4, i64 %58
  %60 = bitcast float* %59 to <4 x float>*
  %wide.load.8 = load <4 x float>, <4 x float>* %60, align 4, !tbaa !115
  %61 = fadd <4 x float> %broadcast.splat8, %wide.load.8
  %62 = getelementptr inbounds float, float* %5, i64 %58
  %63 = bitcast float* %62 to <4 x float>*
  store <4 x float> %61, <4 x float>* %63, align 4, !tbaa !118
  %64 = add nuw nsw i64 %10, 36
  %65 = getelementptr inbounds float, float* %4, i64 %64
  %66 = bitcast float* %65 to <4 x float>*
  %wide.load.9 = load <4 x float>, <4 x float>* %66, align 4, !tbaa !115
  %67 = fadd <4 x float> %broadcast.splat8, %wide.load.9
  %68 = getelementptr inbounds float, float* %5, i64 %64
  %69 = bitcast float* %68 to <4 x float>*
  store <4 x float> %67, <4 x float>* %69, align 4, !tbaa !118
  %70 = add nuw nsw i64 %10, 40
  %71 = getelementptr inbounds float, float* %4, i64 %70
  %72 = bitcast float* %71 to <4 x float>*
  %wide.load.10 = load <4 x float>, <4 x float>* %72, align 4, !tbaa !115
  %73 = fadd <4 x float> %broadcast.splat8, %wide.load.10
  %74 = getelementptr inbounds float, float* %5, i64 %70
  %75 = bitcast float* %74 to <4 x float>*
  store <4 x float> %73, <4 x float>* %75, align 4, !tbaa !118
  %76 = add nuw nsw i64 %10, 44
  %77 = getelementptr inbounds float, float* %4, i64 %76
  %78 = bitcast float* %77 to <4 x float>*
  %wide.load.11 = load <4 x float>, <4 x float>* %78, align 4, !tbaa !115
  %79 = fadd <4 x float> %broadcast.splat8, %wide.load.11
  %80 = getelementptr inbounds float, float* %5, i64 %76
  %81 = bitcast float* %80 to <4 x float>*
  store <4 x float> %79, <4 x float>* %81, align 4, !tbaa !118
  %82 = add nuw nsw i64 %10, 48
  %83 = getelementptr inbounds float, float* %4, i64 %82
  %84 = bitcast float* %83 to <4 x float>*
  %wide.load.12 = load <4 x float>, <4 x float>* %84, align 4, !tbaa !115
  %85 = fadd <4 x float> %broadcast.splat8, %wide.load.12
  %86 = getelementptr inbounds float, float* %5, i64 %82
  %87 = bitcast float* %86 to <4 x float>*
  store <4 x float> %85, <4 x float>* %87, align 4, !tbaa !118
  %88 = add nuw nsw i64 %10, 52
  %89 = getelementptr inbounds float, float* %4, i64 %88
  %90 = bitcast float* %89 to <4 x float>*
  %wide.load.13 = load <4 x float>, <4 x float>* %90, align 4, !tbaa !115
  %91 = fadd <4 x float> %broadcast.splat8, %wide.load.13
  %92 = getelementptr inbounds float, float* %5, i64 %88
  %93 = bitcast float* %92 to <4 x float>*
  store <4 x float> %91, <4 x float>* %93, align 4, !tbaa !118
  %94 = add nuw nsw i64 %10, 56
  %95 = getelementptr inbounds float, float* %4, i64 %94
  %96 = bitcast float* %95 to <4 x float>*
  %wide.load.14 = load <4 x float>, <4 x float>* %96, align 4, !tbaa !115
  %97 = fadd <4 x float> %broadcast.splat8, %wide.load.14
  %98 = getelementptr inbounds float, float* %5, i64 %94
  %99 = bitcast float* %98 to <4 x float>*
  store <4 x float> %97, <4 x float>* %99, align 4, !tbaa !118
  %100 = add nuw nsw i64 %10, 60
  %101 = getelementptr inbounds float, float* %4, i64 %100
  %102 = bitcast float* %101 to <4 x float>*
  %wide.load.15 = load <4 x float>, <4 x float>* %102, align 4, !tbaa !115
  %103 = fadd <4 x float> %broadcast.splat8, %wide.load.15
  %104 = getelementptr inbounds float, float* %5, i64 %100
  %105 = bitcast float* %104 to <4 x float>*
  store <4 x float> %103, <4 x float>* %105, align 4, !tbaa !118
  %106 = add nuw nsw i64 %10, 64
  %107 = getelementptr inbounds float, float* %4, i64 %106
  %108 = bitcast float* %107 to <4 x float>*
  %wide.load.16 = load <4 x float>, <4 x float>* %108, align 4, !tbaa !115
  %109 = fadd <4 x float> %broadcast.splat8, %wide.load.16
  %110 = getelementptr inbounds float, float* %5, i64 %106
  %111 = bitcast float* %110 to <4 x float>*
  store <4 x float> %109, <4 x float>* %111, align 4, !tbaa !118
  %112 = add nuw nsw i64 %10, 68
  %113 = getelementptr inbounds float, float* %4, i64 %112
  %114 = bitcast float* %113 to <4 x float>*
  %wide.load.17 = load <4 x float>, <4 x float>* %114, align 4, !tbaa !115
  %115 = fadd <4 x float> %broadcast.splat8, %wide.load.17
  %116 = getelementptr inbounds float, float* %5, i64 %112
  %117 = bitcast float* %116 to <4 x float>*
  store <4 x float> %115, <4 x float>* %117, align 4, !tbaa !118
  %118 = add nuw nsw i64 %10, 72
  %119 = getelementptr inbounds float, float* %4, i64 %118
  %120 = bitcast float* %119 to <4 x float>*
  %wide.load.18 = load <4 x float>, <4 x float>* %120, align 4, !tbaa !115
  %121 = fadd <4 x float> %broadcast.splat8, %wide.load.18
  %122 = getelementptr inbounds float, float* %5, i64 %118
  %123 = bitcast float* %122 to <4 x float>*
  store <4 x float> %121, <4 x float>* %123, align 4, !tbaa !118
  %124 = add nuw nsw i64 %10, 76
  %125 = getelementptr inbounds float, float* %4, i64 %124
  %126 = bitcast float* %125 to <4 x float>*
  %wide.load.19 = load <4 x float>, <4 x float>* %126, align 4, !tbaa !115
  %127 = fadd <4 x float> %broadcast.splat8, %wide.load.19
  %128 = getelementptr inbounds float, float* %5, i64 %124
  %129 = bitcast float* %128 to <4 x float>*
  store <4 x float> %127, <4 x float>* %129, align 4, !tbaa !118
  %130 = add nuw nsw i64 %10, 80
  %131 = getelementptr inbounds float, float* %4, i64 %130
  %132 = bitcast float* %131 to <4 x float>*
  %wide.load.20 = load <4 x float>, <4 x float>* %132, align 4, !tbaa !115
  %133 = fadd <4 x float> %broadcast.splat8, %wide.load.20
  %134 = getelementptr inbounds float, float* %5, i64 %130
  %135 = bitcast float* %134 to <4 x float>*
  store <4 x float> %133, <4 x float>* %135, align 4, !tbaa !118
  %136 = add nuw nsw i64 %10, 84
  %137 = getelementptr inbounds float, float* %4, i64 %136
  %138 = bitcast float* %137 to <4 x float>*
  %wide.load.21 = load <4 x float>, <4 x float>* %138, align 4, !tbaa !115
  %139 = fadd <4 x float> %broadcast.splat8, %wide.load.21
  %140 = getelementptr inbounds float, float* %5, i64 %136
  %141 = bitcast float* %140 to <4 x float>*
  store <4 x float> %139, <4 x float>* %141, align 4, !tbaa !118
  %142 = add nuw nsw i64 %10, 88
  %143 = getelementptr inbounds float, float* %4, i64 %142
  %144 = bitcast float* %143 to <4 x float>*
  %wide.load.22 = load <4 x float>, <4 x float>* %144, align 4, !tbaa !115
  %145 = fadd <4 x float> %broadcast.splat8, %wide.load.22
  %146 = getelementptr inbounds float, float* %5, i64 %142
  %147 = bitcast float* %146 to <4 x float>*
  store <4 x float> %145, <4 x float>* %147, align 4, !tbaa !118
  %148 = add nuw nsw i64 %10, 92
  %149 = getelementptr inbounds float, float* %4, i64 %148
  %150 = bitcast float* %149 to <4 x float>*
  %wide.load.23 = load <4 x float>, <4 x float>* %150, align 4, !tbaa !115
  %151 = fadd <4 x float> %broadcast.splat8, %wide.load.23
  %152 = getelementptr inbounds float, float* %5, i64 %148
  %153 = bitcast float* %152 to <4 x float>*
  store <4 x float> %151, <4 x float>* %153, align 4, !tbaa !118
  %154 = add nuw nsw i64 %10, 96
  %155 = getelementptr inbounds float, float* %4, i64 %154
  %156 = bitcast float* %155 to <4 x float>*
  %wide.load.24 = load <4 x float>, <4 x float>* %156, align 4, !tbaa !115
  %157 = fadd <4 x float> %broadcast.splat8, %wide.load.24
  %158 = getelementptr inbounds float, float* %5, i64 %154
  %159 = bitcast float* %158 to <4 x float>*
  store <4 x float> %157, <4 x float>* %159, align 4, !tbaa !118
  %160 = add nuw nsw i64 %10, 100
  %161 = getelementptr inbounds float, float* %4, i64 %160
  %162 = bitcast float* %161 to <4 x float>*
  %wide.load.25 = load <4 x float>, <4 x float>* %162, align 4, !tbaa !115
  %163 = fadd <4 x float> %broadcast.splat8, %wide.load.25
  %164 = getelementptr inbounds float, float* %5, i64 %160
  %165 = bitcast float* %164 to <4 x float>*
  store <4 x float> %163, <4 x float>* %165, align 4, !tbaa !118
  %166 = add nuw nsw i64 %10, 104
  %167 = getelementptr inbounds float, float* %4, i64 %166
  %168 = bitcast float* %167 to <4 x float>*
  %wide.load.26 = load <4 x float>, <4 x float>* %168, align 4, !tbaa !115
  %169 = fadd <4 x float> %broadcast.splat8, %wide.load.26
  %170 = getelementptr inbounds float, float* %5, i64 %166
  %171 = bitcast float* %170 to <4 x float>*
  store <4 x float> %169, <4 x float>* %171, align 4, !tbaa !118
  %172 = add nuw nsw i64 %10, 108
  %173 = getelementptr inbounds float, float* %4, i64 %172
  %174 = load float, float* %173, align 4, !tbaa !115
  %175 = fadd float %8, %174
  %176 = getelementptr inbounds float, float* %5, i64 %172
  store float %175, float* %176, align 4, !tbaa !118
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 109
  br i1 %exitcond3, label %for_end3, label %for_begin4.preheader, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 96
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !28
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_relu_4(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_relu_4_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_relu_4_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %1 to float*
  %3 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv4 = phi i64 [ 0, %entry ], [ %indvars.iv.next5, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv4, 11881
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv1 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next2, %for_begin4.preheader ]
  %5 = mul nuw nsw i64 %indvars.iv1, 109
  %6 = add nuw nsw i64 %5, %4
  %7 = getelementptr inbounds float, float* %2, i64 %6
  %8 = bitcast float* %7 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !121
  %9 = fcmp ogt <4 x float> %wide.load, zeroinitializer
  %10 = select <4 x i1> %9, <4 x float> %wide.load, <4 x float> zeroinitializer
  %11 = getelementptr inbounds float, float* %3, i64 %6
  %12 = bitcast float* %11 to <4 x float>*
  store <4 x float> %10, <4 x float>* %12, align 4, !tbaa !124
  %13 = add nuw nsw i64 %6, 4
  %14 = getelementptr inbounds float, float* %2, i64 %13
  %15 = bitcast float* %14 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %15, align 4, !tbaa !121
  %16 = fcmp ogt <4 x float> %wide.load.1, zeroinitializer
  %17 = select <4 x i1> %16, <4 x float> %wide.load.1, <4 x float> zeroinitializer
  %18 = getelementptr inbounds float, float* %3, i64 %13
  %19 = bitcast float* %18 to <4 x float>*
  store <4 x float> %17, <4 x float>* %19, align 4, !tbaa !124
  %20 = add nuw nsw i64 %6, 8
  %21 = getelementptr inbounds float, float* %2, i64 %20
  %22 = bitcast float* %21 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %22, align 4, !tbaa !121
  %23 = fcmp ogt <4 x float> %wide.load.2, zeroinitializer
  %24 = select <4 x i1> %23, <4 x float> %wide.load.2, <4 x float> zeroinitializer
  %25 = getelementptr inbounds float, float* %3, i64 %20
  %26 = bitcast float* %25 to <4 x float>*
  store <4 x float> %24, <4 x float>* %26, align 4, !tbaa !124
  %27 = add nuw nsw i64 %6, 12
  %28 = getelementptr inbounds float, float* %2, i64 %27
  %29 = bitcast float* %28 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %29, align 4, !tbaa !121
  %30 = fcmp ogt <4 x float> %wide.load.3, zeroinitializer
  %31 = select <4 x i1> %30, <4 x float> %wide.load.3, <4 x float> zeroinitializer
  %32 = getelementptr inbounds float, float* %3, i64 %27
  %33 = bitcast float* %32 to <4 x float>*
  store <4 x float> %31, <4 x float>* %33, align 4, !tbaa !124
  %34 = add nuw nsw i64 %6, 16
  %35 = getelementptr inbounds float, float* %2, i64 %34
  %36 = bitcast float* %35 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %36, align 4, !tbaa !121
  %37 = fcmp ogt <4 x float> %wide.load.4, zeroinitializer
  %38 = select <4 x i1> %37, <4 x float> %wide.load.4, <4 x float> zeroinitializer
  %39 = getelementptr inbounds float, float* %3, i64 %34
  %40 = bitcast float* %39 to <4 x float>*
  store <4 x float> %38, <4 x float>* %40, align 4, !tbaa !124
  %41 = add nuw nsw i64 %6, 20
  %42 = getelementptr inbounds float, float* %2, i64 %41
  %43 = bitcast float* %42 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %43, align 4, !tbaa !121
  %44 = fcmp ogt <4 x float> %wide.load.5, zeroinitializer
  %45 = select <4 x i1> %44, <4 x float> %wide.load.5, <4 x float> zeroinitializer
  %46 = getelementptr inbounds float, float* %3, i64 %41
  %47 = bitcast float* %46 to <4 x float>*
  store <4 x float> %45, <4 x float>* %47, align 4, !tbaa !124
  %48 = add nuw nsw i64 %6, 24
  %49 = getelementptr inbounds float, float* %2, i64 %48
  %50 = bitcast float* %49 to <4 x float>*
  %wide.load.6 = load <4 x float>, <4 x float>* %50, align 4, !tbaa !121
  %51 = fcmp ogt <4 x float> %wide.load.6, zeroinitializer
  %52 = select <4 x i1> %51, <4 x float> %wide.load.6, <4 x float> zeroinitializer
  %53 = getelementptr inbounds float, float* %3, i64 %48
  %54 = bitcast float* %53 to <4 x float>*
  store <4 x float> %52, <4 x float>* %54, align 4, !tbaa !124
  %55 = add nuw nsw i64 %6, 28
  %56 = getelementptr inbounds float, float* %2, i64 %55
  %57 = bitcast float* %56 to <4 x float>*
  %wide.load.7 = load <4 x float>, <4 x float>* %57, align 4, !tbaa !121
  %58 = fcmp ogt <4 x float> %wide.load.7, zeroinitializer
  %59 = select <4 x i1> %58, <4 x float> %wide.load.7, <4 x float> zeroinitializer
  %60 = getelementptr inbounds float, float* %3, i64 %55
  %61 = bitcast float* %60 to <4 x float>*
  store <4 x float> %59, <4 x float>* %61, align 4, !tbaa !124
  %62 = add nuw nsw i64 %6, 32
  %63 = getelementptr inbounds float, float* %2, i64 %62
  %64 = bitcast float* %63 to <4 x float>*
  %wide.load.8 = load <4 x float>, <4 x float>* %64, align 4, !tbaa !121
  %65 = fcmp ogt <4 x float> %wide.load.8, zeroinitializer
  %66 = select <4 x i1> %65, <4 x float> %wide.load.8, <4 x float> zeroinitializer
  %67 = getelementptr inbounds float, float* %3, i64 %62
  %68 = bitcast float* %67 to <4 x float>*
  store <4 x float> %66, <4 x float>* %68, align 4, !tbaa !124
  %69 = add nuw nsw i64 %6, 36
  %70 = getelementptr inbounds float, float* %2, i64 %69
  %71 = bitcast float* %70 to <4 x float>*
  %wide.load.9 = load <4 x float>, <4 x float>* %71, align 4, !tbaa !121
  %72 = fcmp ogt <4 x float> %wide.load.9, zeroinitializer
  %73 = select <4 x i1> %72, <4 x float> %wide.load.9, <4 x float> zeroinitializer
  %74 = getelementptr inbounds float, float* %3, i64 %69
  %75 = bitcast float* %74 to <4 x float>*
  store <4 x float> %73, <4 x float>* %75, align 4, !tbaa !124
  %76 = add nuw nsw i64 %6, 40
  %77 = getelementptr inbounds float, float* %2, i64 %76
  %78 = bitcast float* %77 to <4 x float>*
  %wide.load.10 = load <4 x float>, <4 x float>* %78, align 4, !tbaa !121
  %79 = fcmp ogt <4 x float> %wide.load.10, zeroinitializer
  %80 = select <4 x i1> %79, <4 x float> %wide.load.10, <4 x float> zeroinitializer
  %81 = getelementptr inbounds float, float* %3, i64 %76
  %82 = bitcast float* %81 to <4 x float>*
  store <4 x float> %80, <4 x float>* %82, align 4, !tbaa !124
  %83 = add nuw nsw i64 %6, 44
  %84 = getelementptr inbounds float, float* %2, i64 %83
  %85 = bitcast float* %84 to <4 x float>*
  %wide.load.11 = load <4 x float>, <4 x float>* %85, align 4, !tbaa !121
  %86 = fcmp ogt <4 x float> %wide.load.11, zeroinitializer
  %87 = select <4 x i1> %86, <4 x float> %wide.load.11, <4 x float> zeroinitializer
  %88 = getelementptr inbounds float, float* %3, i64 %83
  %89 = bitcast float* %88 to <4 x float>*
  store <4 x float> %87, <4 x float>* %89, align 4, !tbaa !124
  %90 = add nuw nsw i64 %6, 48
  %91 = getelementptr inbounds float, float* %2, i64 %90
  %92 = bitcast float* %91 to <4 x float>*
  %wide.load.12 = load <4 x float>, <4 x float>* %92, align 4, !tbaa !121
  %93 = fcmp ogt <4 x float> %wide.load.12, zeroinitializer
  %94 = select <4 x i1> %93, <4 x float> %wide.load.12, <4 x float> zeroinitializer
  %95 = getelementptr inbounds float, float* %3, i64 %90
  %96 = bitcast float* %95 to <4 x float>*
  store <4 x float> %94, <4 x float>* %96, align 4, !tbaa !124
  %97 = add nuw nsw i64 %6, 52
  %98 = getelementptr inbounds float, float* %2, i64 %97
  %99 = bitcast float* %98 to <4 x float>*
  %wide.load.13 = load <4 x float>, <4 x float>* %99, align 4, !tbaa !121
  %100 = fcmp ogt <4 x float> %wide.load.13, zeroinitializer
  %101 = select <4 x i1> %100, <4 x float> %wide.load.13, <4 x float> zeroinitializer
  %102 = getelementptr inbounds float, float* %3, i64 %97
  %103 = bitcast float* %102 to <4 x float>*
  store <4 x float> %101, <4 x float>* %103, align 4, !tbaa !124
  %104 = add nuw nsw i64 %6, 56
  %105 = getelementptr inbounds float, float* %2, i64 %104
  %106 = bitcast float* %105 to <4 x float>*
  %wide.load.14 = load <4 x float>, <4 x float>* %106, align 4, !tbaa !121
  %107 = fcmp ogt <4 x float> %wide.load.14, zeroinitializer
  %108 = select <4 x i1> %107, <4 x float> %wide.load.14, <4 x float> zeroinitializer
  %109 = getelementptr inbounds float, float* %3, i64 %104
  %110 = bitcast float* %109 to <4 x float>*
  store <4 x float> %108, <4 x float>* %110, align 4, !tbaa !124
  %111 = add nuw nsw i64 %6, 60
  %112 = getelementptr inbounds float, float* %2, i64 %111
  %113 = bitcast float* %112 to <4 x float>*
  %wide.load.15 = load <4 x float>, <4 x float>* %113, align 4, !tbaa !121
  %114 = fcmp ogt <4 x float> %wide.load.15, zeroinitializer
  %115 = select <4 x i1> %114, <4 x float> %wide.load.15, <4 x float> zeroinitializer
  %116 = getelementptr inbounds float, float* %3, i64 %111
  %117 = bitcast float* %116 to <4 x float>*
  store <4 x float> %115, <4 x float>* %117, align 4, !tbaa !124
  %118 = add nuw nsw i64 %6, 64
  %119 = getelementptr inbounds float, float* %2, i64 %118
  %120 = bitcast float* %119 to <4 x float>*
  %wide.load.16 = load <4 x float>, <4 x float>* %120, align 4, !tbaa !121
  %121 = fcmp ogt <4 x float> %wide.load.16, zeroinitializer
  %122 = select <4 x i1> %121, <4 x float> %wide.load.16, <4 x float> zeroinitializer
  %123 = getelementptr inbounds float, float* %3, i64 %118
  %124 = bitcast float* %123 to <4 x float>*
  store <4 x float> %122, <4 x float>* %124, align 4, !tbaa !124
  %125 = add nuw nsw i64 %6, 68
  %126 = getelementptr inbounds float, float* %2, i64 %125
  %127 = bitcast float* %126 to <4 x float>*
  %wide.load.17 = load <4 x float>, <4 x float>* %127, align 4, !tbaa !121
  %128 = fcmp ogt <4 x float> %wide.load.17, zeroinitializer
  %129 = select <4 x i1> %128, <4 x float> %wide.load.17, <4 x float> zeroinitializer
  %130 = getelementptr inbounds float, float* %3, i64 %125
  %131 = bitcast float* %130 to <4 x float>*
  store <4 x float> %129, <4 x float>* %131, align 4, !tbaa !124
  %132 = add nuw nsw i64 %6, 72
  %133 = getelementptr inbounds float, float* %2, i64 %132
  %134 = bitcast float* %133 to <4 x float>*
  %wide.load.18 = load <4 x float>, <4 x float>* %134, align 4, !tbaa !121
  %135 = fcmp ogt <4 x float> %wide.load.18, zeroinitializer
  %136 = select <4 x i1> %135, <4 x float> %wide.load.18, <4 x float> zeroinitializer
  %137 = getelementptr inbounds float, float* %3, i64 %132
  %138 = bitcast float* %137 to <4 x float>*
  store <4 x float> %136, <4 x float>* %138, align 4, !tbaa !124
  %139 = add nuw nsw i64 %6, 76
  %140 = getelementptr inbounds float, float* %2, i64 %139
  %141 = bitcast float* %140 to <4 x float>*
  %wide.load.19 = load <4 x float>, <4 x float>* %141, align 4, !tbaa !121
  %142 = fcmp ogt <4 x float> %wide.load.19, zeroinitializer
  %143 = select <4 x i1> %142, <4 x float> %wide.load.19, <4 x float> zeroinitializer
  %144 = getelementptr inbounds float, float* %3, i64 %139
  %145 = bitcast float* %144 to <4 x float>*
  store <4 x float> %143, <4 x float>* %145, align 4, !tbaa !124
  %146 = add nuw nsw i64 %6, 80
  %147 = getelementptr inbounds float, float* %2, i64 %146
  %148 = bitcast float* %147 to <4 x float>*
  %wide.load.20 = load <4 x float>, <4 x float>* %148, align 4, !tbaa !121
  %149 = fcmp ogt <4 x float> %wide.load.20, zeroinitializer
  %150 = select <4 x i1> %149, <4 x float> %wide.load.20, <4 x float> zeroinitializer
  %151 = getelementptr inbounds float, float* %3, i64 %146
  %152 = bitcast float* %151 to <4 x float>*
  store <4 x float> %150, <4 x float>* %152, align 4, !tbaa !124
  %153 = add nuw nsw i64 %6, 84
  %154 = getelementptr inbounds float, float* %2, i64 %153
  %155 = bitcast float* %154 to <4 x float>*
  %wide.load.21 = load <4 x float>, <4 x float>* %155, align 4, !tbaa !121
  %156 = fcmp ogt <4 x float> %wide.load.21, zeroinitializer
  %157 = select <4 x i1> %156, <4 x float> %wide.load.21, <4 x float> zeroinitializer
  %158 = getelementptr inbounds float, float* %3, i64 %153
  %159 = bitcast float* %158 to <4 x float>*
  store <4 x float> %157, <4 x float>* %159, align 4, !tbaa !124
  %160 = add nuw nsw i64 %6, 88
  %161 = getelementptr inbounds float, float* %2, i64 %160
  %162 = bitcast float* %161 to <4 x float>*
  %wide.load.22 = load <4 x float>, <4 x float>* %162, align 4, !tbaa !121
  %163 = fcmp ogt <4 x float> %wide.load.22, zeroinitializer
  %164 = select <4 x i1> %163, <4 x float> %wide.load.22, <4 x float> zeroinitializer
  %165 = getelementptr inbounds float, float* %3, i64 %160
  %166 = bitcast float* %165 to <4 x float>*
  store <4 x float> %164, <4 x float>* %166, align 4, !tbaa !124
  %167 = add nuw nsw i64 %6, 92
  %168 = getelementptr inbounds float, float* %2, i64 %167
  %169 = bitcast float* %168 to <4 x float>*
  %wide.load.23 = load <4 x float>, <4 x float>* %169, align 4, !tbaa !121
  %170 = fcmp ogt <4 x float> %wide.load.23, zeroinitializer
  %171 = select <4 x i1> %170, <4 x float> %wide.load.23, <4 x float> zeroinitializer
  %172 = getelementptr inbounds float, float* %3, i64 %167
  %173 = bitcast float* %172 to <4 x float>*
  store <4 x float> %171, <4 x float>* %173, align 4, !tbaa !124
  %174 = add nuw nsw i64 %6, 96
  %175 = getelementptr inbounds float, float* %2, i64 %174
  %176 = bitcast float* %175 to <4 x float>*
  %wide.load.24 = load <4 x float>, <4 x float>* %176, align 4, !tbaa !121
  %177 = fcmp ogt <4 x float> %wide.load.24, zeroinitializer
  %178 = select <4 x i1> %177, <4 x float> %wide.load.24, <4 x float> zeroinitializer
  %179 = getelementptr inbounds float, float* %3, i64 %174
  %180 = bitcast float* %179 to <4 x float>*
  store <4 x float> %178, <4 x float>* %180, align 4, !tbaa !124
  %181 = add nuw nsw i64 %6, 100
  %182 = getelementptr inbounds float, float* %2, i64 %181
  %183 = bitcast float* %182 to <4 x float>*
  %wide.load.25 = load <4 x float>, <4 x float>* %183, align 4, !tbaa !121
  %184 = fcmp ogt <4 x float> %wide.load.25, zeroinitializer
  %185 = select <4 x i1> %184, <4 x float> %wide.load.25, <4 x float> zeroinitializer
  %186 = getelementptr inbounds float, float* %3, i64 %181
  %187 = bitcast float* %186 to <4 x float>*
  store <4 x float> %185, <4 x float>* %187, align 4, !tbaa !124
  %188 = add nuw nsw i64 %6, 104
  %189 = getelementptr inbounds float, float* %2, i64 %188
  %190 = bitcast float* %189 to <4 x float>*
  %wide.load.26 = load <4 x float>, <4 x float>* %190, align 4, !tbaa !121
  %191 = fcmp ogt <4 x float> %wide.load.26, zeroinitializer
  %192 = select <4 x i1> %191, <4 x float> %wide.load.26, <4 x float> zeroinitializer
  %193 = getelementptr inbounds float, float* %3, i64 %188
  %194 = bitcast float* %193 to <4 x float>*
  store <4 x float> %192, <4 x float>* %194, align 4, !tbaa !124
  %195 = add nuw nsw i64 %6, 108
  %196 = getelementptr inbounds float, float* %2, i64 %195
  %197 = load float, float* %196, align 4, !tbaa !121
  %198 = fcmp ogt float %197, 0.000000e+00
  %199 = select i1 %198, float %197, float 0.000000e+00
  %200 = getelementptr inbounds float, float* %3, i64 %195
  store float %199, float* %200, align 4, !tbaa !124
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 109
  br i1 %exitcond3, label %for_end3, label %for_begin4.preheader, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 96
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !28
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
  %4 = mul nuw nsw i64 %indvars.iv6, 144
  %5 = mul nuw nsw i64 %indvars.iv6, 625
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv3 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next4, %for_begin4.preheader ]
  %6 = mul nuw nsw i64 %indvars.iv3, 12
  %7 = add nuw nsw i64 %6, %4
  %8 = mul nuw nsw i64 %indvars.iv3, 50
  %9 = add nuw nsw i64 %8, %5
  %10 = getelementptr inbounds float, float* %2, i64 %7
  %11 = getelementptr inbounds float, float* %3, i64 %9
  %12 = bitcast float* %11 to <8 x float>*
  %wide.vec = load <8 x float>, <8 x float>* %12, align 4, !tbaa !127
  %strided.vec = shufflevector <8 x float> %wide.vec, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %13 = fcmp olt <4 x float> %strided.vec, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %14 = select <4 x i1> %13, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec
  %15 = add nuw nsw i64 %9, 1
  %16 = getelementptr inbounds float, float* %3, i64 %15
  %17 = bitcast float* %16 to <8 x float>*
  %wide.vec9 = load <8 x float>, <8 x float>* %17, align 4, !tbaa !127
  %strided.vec10 = shufflevector <8 x float> %wide.vec9, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec11 = shufflevector <8 x float> %wide.vec9, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %18 = fcmp ogt <4 x float> %14, %strided.vec10
  %19 = select <4 x i1> %18, <4 x float> %14, <4 x float> %strided.vec10
  %20 = fcmp ogt <4 x float> %19, %strided.vec11
  %21 = select <4 x i1> %20, <4 x float> %19, <4 x float> %strided.vec11
  %22 = add nuw nsw i64 %9, 25
  %23 = getelementptr inbounds float, float* %3, i64 %22
  %24 = bitcast float* %23 to <8 x float>*
  %wide.vec12 = load <8 x float>, <8 x float>* %24, align 4, !tbaa !127
  %strided.vec13 = shufflevector <8 x float> %wide.vec12, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %25 = fcmp ogt <4 x float> %21, %strided.vec13
  %26 = select <4 x i1> %25, <4 x float> %21, <4 x float> %strided.vec13
  %27 = add nuw nsw i64 %9, 26
  %28 = getelementptr inbounds float, float* %3, i64 %27
  %29 = bitcast float* %28 to <8 x float>*
  %wide.vec14 = load <8 x float>, <8 x float>* %29, align 4, !tbaa !127
  %strided.vec15 = shufflevector <8 x float> %wide.vec14, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec16 = shufflevector <8 x float> %wide.vec14, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %30 = fcmp ogt <4 x float> %26, %strided.vec15
  %31 = select <4 x i1> %30, <4 x float> %26, <4 x float> %strided.vec15
  %32 = fcmp ogt <4 x float> %31, %strided.vec16
  %33 = select <4 x i1> %32, <4 x float> %31, <4 x float> %strided.vec16
  %34 = add nuw nsw i64 %9, 50
  %35 = getelementptr inbounds float, float* %3, i64 %34
  %36 = bitcast float* %35 to <8 x float>*
  %wide.vec17 = load <8 x float>, <8 x float>* %36, align 4, !tbaa !127
  %strided.vec18 = shufflevector <8 x float> %wide.vec17, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %37 = fcmp ogt <4 x float> %33, %strided.vec18
  %38 = select <4 x i1> %37, <4 x float> %33, <4 x float> %strided.vec18
  %39 = add nuw nsw i64 %9, 51
  %40 = getelementptr inbounds float, float* %3, i64 %39
  %41 = bitcast float* %40 to <8 x float>*
  %wide.vec19 = load <8 x float>, <8 x float>* %41, align 4, !tbaa !127
  %strided.vec20 = shufflevector <8 x float> %wide.vec19, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec21 = shufflevector <8 x float> %wide.vec19, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %42 = fcmp ogt <4 x float> %38, %strided.vec20
  %43 = select <4 x i1> %42, <4 x float> %38, <4 x float> %strided.vec20
  %44 = fcmp ogt <4 x float> %43, %strided.vec21
  %45 = select <4 x i1> %44, <4 x float> %43, <4 x float> %strided.vec21
  %46 = bitcast float* %10 to <4 x float>*
  store <4 x float> %45, <4 x float>* %46, align 4, !tbaa !130
  %47 = add nuw nsw i64 %7, 4
  %48 = getelementptr inbounds float, float* %2, i64 %47
  %49 = add nuw nsw i64 %9, 8
  %50 = getelementptr inbounds float, float* %3, i64 %49
  %51 = bitcast float* %50 to <8 x float>*
  %wide.vec.1 = load <8 x float>, <8 x float>* %51, align 4, !tbaa !127
  %strided.vec.1 = shufflevector <8 x float> %wide.vec.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %52 = fcmp olt <4 x float> %strided.vec.1, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %53 = select <4 x i1> %52, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.1
  %54 = add nuw nsw i64 %9, 9
  %55 = getelementptr inbounds float, float* %3, i64 %54
  %56 = bitcast float* %55 to <8 x float>*
  %wide.vec9.1 = load <8 x float>, <8 x float>* %56, align 4, !tbaa !127
  %strided.vec10.1 = shufflevector <8 x float> %wide.vec9.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec11.1 = shufflevector <8 x float> %wide.vec9.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %57 = fcmp ogt <4 x float> %53, %strided.vec10.1
  %58 = select <4 x i1> %57, <4 x float> %53, <4 x float> %strided.vec10.1
  %59 = fcmp ogt <4 x float> %58, %strided.vec11.1
  %60 = select <4 x i1> %59, <4 x float> %58, <4 x float> %strided.vec11.1
  %61 = add nuw nsw i64 %9, 33
  %62 = getelementptr inbounds float, float* %3, i64 %61
  %63 = bitcast float* %62 to <8 x float>*
  %wide.vec12.1 = load <8 x float>, <8 x float>* %63, align 4, !tbaa !127
  %strided.vec13.1 = shufflevector <8 x float> %wide.vec12.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %64 = fcmp ogt <4 x float> %60, %strided.vec13.1
  %65 = select <4 x i1> %64, <4 x float> %60, <4 x float> %strided.vec13.1
  %66 = add nuw nsw i64 %9, 34
  %67 = getelementptr inbounds float, float* %3, i64 %66
  %68 = bitcast float* %67 to <8 x float>*
  %wide.vec14.1 = load <8 x float>, <8 x float>* %68, align 4, !tbaa !127
  %strided.vec15.1 = shufflevector <8 x float> %wide.vec14.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec16.1 = shufflevector <8 x float> %wide.vec14.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %69 = fcmp ogt <4 x float> %65, %strided.vec15.1
  %70 = select <4 x i1> %69, <4 x float> %65, <4 x float> %strided.vec15.1
  %71 = fcmp ogt <4 x float> %70, %strided.vec16.1
  %72 = select <4 x i1> %71, <4 x float> %70, <4 x float> %strided.vec16.1
  %73 = add nuw nsw i64 %9, 58
  %74 = getelementptr inbounds float, float* %3, i64 %73
  %75 = bitcast float* %74 to <8 x float>*
  %wide.vec17.1 = load <8 x float>, <8 x float>* %75, align 4, !tbaa !127
  %strided.vec18.1 = shufflevector <8 x float> %wide.vec17.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %76 = fcmp ogt <4 x float> %72, %strided.vec18.1
  %77 = select <4 x i1> %76, <4 x float> %72, <4 x float> %strided.vec18.1
  %78 = add nuw nsw i64 %9, 59
  %79 = getelementptr inbounds float, float* %3, i64 %78
  %80 = bitcast float* %79 to <8 x float>*
  %wide.vec19.1 = load <8 x float>, <8 x float>* %80, align 4, !tbaa !127
  %strided.vec20.1 = shufflevector <8 x float> %wide.vec19.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec21.1 = shufflevector <8 x float> %wide.vec19.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %81 = fcmp ogt <4 x float> %77, %strided.vec20.1
  %82 = select <4 x i1> %81, <4 x float> %77, <4 x float> %strided.vec20.1
  %83 = fcmp ogt <4 x float> %82, %strided.vec21.1
  %84 = select <4 x i1> %83, <4 x float> %82, <4 x float> %strided.vec21.1
  %85 = bitcast float* %48 to <4 x float>*
  store <4 x float> %84, <4 x float>* %85, align 4, !tbaa !130
  %86 = add nuw nsw i64 %7, 8
  %87 = getelementptr inbounds float, float* %2, i64 %86
  %88 = add nuw nsw i64 %9, 16
  %89 = getelementptr inbounds float, float* %3, i64 %88
  %90 = load float, float* %89, align 4, !tbaa !127
  %91 = fcmp olt float %90, 0xC7EFFFFFE0000000
  %92 = select i1 %91, float 0xC7EFFFFFE0000000, float %90
  %93 = add nuw nsw i64 %9, 17
  %94 = getelementptr inbounds float, float* %3, i64 %93
  %95 = load float, float* %94, align 4, !tbaa !127
  %96 = fcmp ogt float %92, %95
  %97 = select i1 %96, float %92, float %95
  %98 = add nuw nsw i64 %9, 18
  %99 = getelementptr inbounds float, float* %3, i64 %98
  %100 = load float, float* %99, align 4, !tbaa !127
  %101 = fcmp ogt float %97, %100
  %102 = select i1 %101, float %97, float %100
  %103 = add nuw nsw i64 %9, 41
  %104 = getelementptr inbounds float, float* %3, i64 %103
  %105 = load float, float* %104, align 4, !tbaa !127
  %106 = fcmp ogt float %102, %105
  %107 = select i1 %106, float %102, float %105
  %108 = add nuw nsw i64 %9, 42
  %109 = getelementptr inbounds float, float* %3, i64 %108
  %110 = load float, float* %109, align 4, !tbaa !127
  %111 = fcmp ogt float %107, %110
  %112 = select i1 %111, float %107, float %110
  %113 = add nuw nsw i64 %9, 43
  %114 = getelementptr inbounds float, float* %3, i64 %113
  %115 = load float, float* %114, align 4, !tbaa !127
  %116 = fcmp ogt float %112, %115
  %117 = select i1 %116, float %112, float %115
  %118 = add nuw nsw i64 %9, 66
  %119 = getelementptr inbounds float, float* %3, i64 %118
  %120 = load float, float* %119, align 4, !tbaa !127
  %121 = fcmp ogt float %117, %120
  %122 = select i1 %121, float %117, float %120
  %123 = add nuw nsw i64 %9, 67
  %124 = getelementptr inbounds float, float* %3, i64 %123
  %125 = load float, float* %124, align 4, !tbaa !127
  %126 = fcmp ogt float %122, %125
  %127 = select i1 %126, float %122, float %125
  %128 = add nuw nsw i64 %9, 68
  %129 = getelementptr inbounds float, float* %3, i64 %128
  %130 = load float, float* %129, align 4, !tbaa !127
  %131 = fcmp ogt float %127, %130
  %132 = select i1 %131, float %127, float %130
  store float %132, float* %87, align 4, !tbaa !130
  %133 = add nuw nsw i64 %7, 9
  %134 = getelementptr inbounds float, float* %2, i64 %133
  %135 = add nuw nsw i64 %9, 18
  %136 = getelementptr inbounds float, float* %3, i64 %135
  %137 = load float, float* %136, align 4, !tbaa !127
  %138 = fcmp olt float %137, 0xC7EFFFFFE0000000
  %139 = select i1 %138, float 0xC7EFFFFFE0000000, float %137
  %140 = add nuw nsw i64 %9, 19
  %141 = getelementptr inbounds float, float* %3, i64 %140
  %142 = load float, float* %141, align 4, !tbaa !127
  %143 = fcmp ogt float %139, %142
  %144 = select i1 %143, float %139, float %142
  %145 = add nuw nsw i64 %9, 20
  %146 = getelementptr inbounds float, float* %3, i64 %145
  %147 = load float, float* %146, align 4, !tbaa !127
  %148 = fcmp ogt float %144, %147
  %149 = select i1 %148, float %144, float %147
  %150 = add nuw nsw i64 %9, 43
  %151 = getelementptr inbounds float, float* %3, i64 %150
  %152 = load float, float* %151, align 4, !tbaa !127
  %153 = fcmp ogt float %149, %152
  %154 = select i1 %153, float %149, float %152
  %155 = add nuw nsw i64 %9, 44
  %156 = getelementptr inbounds float, float* %3, i64 %155
  %157 = load float, float* %156, align 4, !tbaa !127
  %158 = fcmp ogt float %154, %157
  %159 = select i1 %158, float %154, float %157
  %160 = add nuw nsw i64 %9, 45
  %161 = getelementptr inbounds float, float* %3, i64 %160
  %162 = load float, float* %161, align 4, !tbaa !127
  %163 = fcmp ogt float %159, %162
  %164 = select i1 %163, float %159, float %162
  %165 = add nuw nsw i64 %9, 68
  %166 = getelementptr inbounds float, float* %3, i64 %165
  %167 = load float, float* %166, align 4, !tbaa !127
  %168 = fcmp ogt float %164, %167
  %169 = select i1 %168, float %164, float %167
  %170 = add nuw nsw i64 %9, 69
  %171 = getelementptr inbounds float, float* %3, i64 %170
  %172 = load float, float* %171, align 4, !tbaa !127
  %173 = fcmp ogt float %169, %172
  %174 = select i1 %173, float %169, float %172
  %175 = add nuw nsw i64 %9, 70
  %176 = getelementptr inbounds float, float* %3, i64 %175
  %177 = load float, float* %176, align 4, !tbaa !127
  %178 = fcmp ogt float %174, %177
  %179 = select i1 %178, float %174, float %177
  store float %179, float* %134, align 4, !tbaa !130
  %180 = add nuw nsw i64 %7, 10
  %181 = getelementptr inbounds float, float* %2, i64 %180
  %182 = add nuw nsw i64 %9, 20
  %183 = getelementptr inbounds float, float* %3, i64 %182
  %184 = load float, float* %183, align 4, !tbaa !127
  %185 = fcmp olt float %184, 0xC7EFFFFFE0000000
  %186 = select i1 %185, float 0xC7EFFFFFE0000000, float %184
  %187 = add nuw nsw i64 %9, 21
  %188 = getelementptr inbounds float, float* %3, i64 %187
  %189 = load float, float* %188, align 4, !tbaa !127
  %190 = fcmp ogt float %186, %189
  %191 = select i1 %190, float %186, float %189
  %192 = add nuw nsw i64 %9, 22
  %193 = getelementptr inbounds float, float* %3, i64 %192
  %194 = load float, float* %193, align 4, !tbaa !127
  %195 = fcmp ogt float %191, %194
  %196 = select i1 %195, float %191, float %194
  %197 = add nuw nsw i64 %9, 45
  %198 = getelementptr inbounds float, float* %3, i64 %197
  %199 = load float, float* %198, align 4, !tbaa !127
  %200 = fcmp ogt float %196, %199
  %201 = select i1 %200, float %196, float %199
  %202 = add nuw nsw i64 %9, 46
  %203 = getelementptr inbounds float, float* %3, i64 %202
  %204 = load float, float* %203, align 4, !tbaa !127
  %205 = fcmp ogt float %201, %204
  %206 = select i1 %205, float %201, float %204
  %207 = add nuw nsw i64 %9, 47
  %208 = getelementptr inbounds float, float* %3, i64 %207
  %209 = load float, float* %208, align 4, !tbaa !127
  %210 = fcmp ogt float %206, %209
  %211 = select i1 %210, float %206, float %209
  %212 = add nuw nsw i64 %9, 70
  %213 = getelementptr inbounds float, float* %3, i64 %212
  %214 = load float, float* %213, align 4, !tbaa !127
  %215 = fcmp ogt float %211, %214
  %216 = select i1 %215, float %211, float %214
  %217 = add nuw nsw i64 %9, 71
  %218 = getelementptr inbounds float, float* %3, i64 %217
  %219 = load float, float* %218, align 4, !tbaa !127
  %220 = fcmp ogt float %216, %219
  %221 = select i1 %220, float %216, float %219
  %222 = add nuw nsw i64 %9, 72
  %223 = getelementptr inbounds float, float* %3, i64 %222
  %224 = load float, float* %223, align 4, !tbaa !127
  %225 = fcmp ogt float %221, %224
  %226 = select i1 %225, float %221, float %224
  store float %226, float* %181, align 4, !tbaa !130
  %227 = add nuw nsw i64 %7, 11
  %228 = getelementptr inbounds float, float* %2, i64 %227
  %229 = add nuw nsw i64 %9, 22
  %230 = getelementptr inbounds float, float* %3, i64 %229
  %231 = load float, float* %230, align 4, !tbaa !127
  %232 = fcmp olt float %231, 0xC7EFFFFFE0000000
  %233 = select i1 %232, float 0xC7EFFFFFE0000000, float %231
  %234 = add nuw nsw i64 %9, 23
  %235 = getelementptr inbounds float, float* %3, i64 %234
  %236 = load float, float* %235, align 4, !tbaa !127
  %237 = fcmp ogt float %233, %236
  %238 = select i1 %237, float %233, float %236
  %239 = add nuw nsw i64 %9, 24
  %240 = getelementptr inbounds float, float* %3, i64 %239
  %241 = load float, float* %240, align 4, !tbaa !127
  %242 = fcmp ogt float %238, %241
  %243 = select i1 %242, float %238, float %241
  %244 = add nuw nsw i64 %9, 47
  %245 = getelementptr inbounds float, float* %3, i64 %244
  %246 = load float, float* %245, align 4, !tbaa !127
  %247 = fcmp ogt float %243, %246
  %248 = select i1 %247, float %243, float %246
  %249 = add nuw nsw i64 %9, 48
  %250 = getelementptr inbounds float, float* %3, i64 %249
  %251 = load float, float* %250, align 4, !tbaa !127
  %252 = fcmp ogt float %248, %251
  %253 = select i1 %252, float %248, float %251
  %254 = add nuw nsw i64 %9, 49
  %255 = getelementptr inbounds float, float* %3, i64 %254
  %256 = load float, float* %255, align 4, !tbaa !127
  %257 = fcmp ogt float %253, %256
  %258 = select i1 %257, float %253, float %256
  %259 = add nuw nsw i64 %9, 72
  %260 = getelementptr inbounds float, float* %3, i64 %259
  %261 = load float, float* %260, align 4, !tbaa !127
  %262 = fcmp ogt float %258, %261
  %263 = select i1 %262, float %258, float %261
  %264 = add nuw nsw i64 %9, 73
  %265 = getelementptr inbounds float, float* %3, i64 %264
  %266 = load float, float* %265, align 4, !tbaa !127
  %267 = fcmp ogt float %263, %266
  %268 = select i1 %267, float %263, float %266
  %269 = add nuw nsw i64 %9, 74
  %270 = getelementptr inbounds float, float* %3, i64 %269
  %271 = load float, float* %270, align 4, !tbaa !127
  %272 = fcmp ogt float %268, %271
  %273 = select i1 %272, float %268, float %271
  store float %273, float* %228, align 4, !tbaa !130
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 12
  br i1 %exitcond5, label %for_end3, label %for_begin4.preheader, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  %exitcond8 = icmp eq i64 %indvars.iv.next7, 256
  br i1 %exitcond8, label %for_end, label %for_begin1.preheader, !prof !28
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
  %3 = alloca [12 x <8 x float>], align 16
  %4 = alloca [294912 x <8 x float>], align 16
  %5 = alloca [100352 x float], align 16
  %6 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar = phi i64 [ 0, %entry ], [ %indvar.next, %for_end3 ]
  %7 = mul nuw nsw i64 %indvar, 112
  %8 = trunc i64 %indvar to i32
  %9 = urem i32 %8, 14
  %10 = udiv i32 %8, 14
  %.off = add nsw i32 %9, -1
  %11 = icmp ult i32 %.off, 12
  br i1 %11, label %for_begin4.preheader.us.preheader, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep142 = getelementptr [100352 x float], [100352 x float]* %5, i64 0, i64 %7
  %scevgep142143 = bitcast float* %scevgep142 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep142143, i8 0, i64 448, i1 false)
  br label %for_end3

for_begin4.preheader.us.preheader:                ; preds = %for_begin1.preheader
  %12 = mul nsw i32 %10, 1152
  %13 = add nsw i32 %12, -13
  %14 = mul nuw nsw i32 %9, 12
  %15 = add i32 %13, %14
  %16 = sext i32 %15 to i64
  %17 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %7
  store float 0.000000e+00, float* %17, align 16, !tbaa !133
  %18 = or i64 %7, 1
  %19 = add nsw i64 %16, 1
  %20 = getelementptr inbounds float, float* %6, i64 %19
  %21 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %18
  %22 = bitcast float* %20 to <4 x i32>*
  %23 = load <4 x i32>, <4 x i32>* %22, align 4, !tbaa !136
  %24 = bitcast float* %21 to <4 x i32>*
  store <4 x i32> %23, <4 x i32>* %24, align 4, !tbaa !133
  %25 = or i64 %7, 5
  %26 = add nsw i64 %16, 5
  %27 = getelementptr inbounds float, float* %6, i64 %26
  %28 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %25
  %29 = bitcast float* %27 to <4 x i32>*
  %30 = load <4 x i32>, <4 x i32>* %29, align 4, !tbaa !136
  %31 = bitcast float* %28 to <4 x i32>*
  store <4 x i32> %30, <4 x i32>* %31, align 4, !tbaa !133
  %32 = or i64 %7, 9
  %33 = add nsw i64 %16, 9
  %34 = getelementptr inbounds float, float* %6, i64 %33
  %35 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %32
  %36 = bitcast float* %34 to <4 x i32>*
  %37 = load <4 x i32>, <4 x i32>* %36, align 4, !tbaa !136
  %38 = bitcast float* %35 to <4 x i32>*
  store <4 x i32> %37, <4 x i32>* %38, align 4, !tbaa !133
  %39 = or i64 %7, 13
  %40 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %39
  store float 0.000000e+00, float* %40, align 4, !tbaa !133
  %41 = or i64 %7, 14
  %42 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %41
  store float 0.000000e+00, float* %42, align 8, !tbaa !133
  %43 = or i64 %7, 15
  %44 = add nsw i64 %16, 145
  %45 = getelementptr inbounds float, float* %6, i64 %44
  %46 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %43
  %47 = bitcast float* %45 to <4 x i32>*
  %48 = load <4 x i32>, <4 x i32>* %47, align 4, !tbaa !136
  %49 = bitcast float* %46 to <4 x i32>*
  store <4 x i32> %48, <4 x i32>* %49, align 4, !tbaa !133
  %50 = add nuw nsw i64 %41, 5
  %51 = add nsw i64 %16, 149
  %52 = getelementptr inbounds float, float* %6, i64 %51
  %53 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %50
  %54 = bitcast float* %52 to <4 x i32>*
  %55 = load <4 x i32>, <4 x i32>* %54, align 4, !tbaa !136
  %56 = bitcast float* %53 to <4 x i32>*
  store <4 x i32> %55, <4 x i32>* %56, align 4, !tbaa !133
  %57 = add nuw nsw i64 %41, 9
  %58 = add nsw i64 %16, 153
  %59 = getelementptr inbounds float, float* %6, i64 %58
  %60 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %57
  %61 = bitcast float* %59 to <4 x i32>*
  %62 = load <4 x i32>, <4 x i32>* %61, align 4, !tbaa !136
  %63 = bitcast float* %60 to <4 x i32>*
  store <4 x i32> %62, <4 x i32>* %63, align 4, !tbaa !133
  %64 = add nuw nsw i64 %41, 13
  %65 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %64
  store float 0.000000e+00, float* %65, align 4, !tbaa !133
  %66 = add nuw nsw i64 %7, 28
  %67 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %66
  store float 0.000000e+00, float* %67, align 16, !tbaa !133
  %68 = or i64 %66, 1
  %69 = add nsw i64 %16, 289
  %70 = getelementptr inbounds float, float* %6, i64 %69
  %71 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %68
  %72 = bitcast float* %70 to <4 x i32>*
  %73 = load <4 x i32>, <4 x i32>* %72, align 4, !tbaa !136
  %74 = bitcast float* %71 to <4 x i32>*
  store <4 x i32> %73, <4 x i32>* %74, align 4, !tbaa !133
  %75 = add nuw nsw i64 %7, 33
  %76 = add nsw i64 %16, 293
  %77 = getelementptr inbounds float, float* %6, i64 %76
  %78 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %75
  %79 = bitcast float* %77 to <4 x i32>*
  %80 = load <4 x i32>, <4 x i32>* %79, align 4, !tbaa !136
  %81 = bitcast float* %78 to <4 x i32>*
  store <4 x i32> %80, <4 x i32>* %81, align 4, !tbaa !133
  %82 = add nuw nsw i64 %7, 37
  %83 = add nsw i64 %16, 297
  %84 = getelementptr inbounds float, float* %6, i64 %83
  %85 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %82
  %86 = bitcast float* %84 to <4 x i32>*
  %87 = load <4 x i32>, <4 x i32>* %86, align 4, !tbaa !136
  %88 = bitcast float* %85 to <4 x i32>*
  store <4 x i32> %87, <4 x i32>* %88, align 4, !tbaa !133
  %89 = add nuw nsw i64 %7, 41
  %90 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %89
  store float 0.000000e+00, float* %90, align 4, !tbaa !133
  %91 = add nuw nsw i64 %7, 42
  %92 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %91
  store float 0.000000e+00, float* %92, align 8, !tbaa !133
  %93 = or i64 %91, 1
  %94 = add nsw i64 %16, 433
  %95 = getelementptr inbounds float, float* %6, i64 %94
  %96 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %93
  %97 = bitcast float* %95 to <4 x i32>*
  %98 = load <4 x i32>, <4 x i32>* %97, align 4, !tbaa !136
  %99 = bitcast float* %96 to <4 x i32>*
  store <4 x i32> %98, <4 x i32>* %99, align 4, !tbaa !133
  %100 = add nuw nsw i64 %7, 47
  %101 = add nsw i64 %16, 437
  %102 = getelementptr inbounds float, float* %6, i64 %101
  %103 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %100
  %104 = bitcast float* %102 to <4 x i32>*
  %105 = load <4 x i32>, <4 x i32>* %104, align 4, !tbaa !136
  %106 = bitcast float* %103 to <4 x i32>*
  store <4 x i32> %105, <4 x i32>* %106, align 4, !tbaa !133
  %107 = add nuw nsw i64 %7, 51
  %108 = add nsw i64 %16, 441
  %109 = getelementptr inbounds float, float* %6, i64 %108
  %110 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %107
  %111 = bitcast float* %109 to <4 x i32>*
  %112 = load <4 x i32>, <4 x i32>* %111, align 4, !tbaa !136
  %113 = bitcast float* %110 to <4 x i32>*
  store <4 x i32> %112, <4 x i32>* %113, align 4, !tbaa !133
  %114 = add nuw nsw i64 %7, 55
  %115 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %114
  store float 0.000000e+00, float* %115, align 4, !tbaa !133
  %116 = add nuw nsw i64 %7, 56
  %117 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %116
  store float 0.000000e+00, float* %117, align 16, !tbaa !133
  %118 = or i64 %116, 1
  %119 = add nsw i64 %16, 577
  %120 = getelementptr inbounds float, float* %6, i64 %119
  %121 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %118
  %122 = bitcast float* %120 to <4 x i32>*
  %123 = load <4 x i32>, <4 x i32>* %122, align 4, !tbaa !136
  %124 = bitcast float* %121 to <4 x i32>*
  store <4 x i32> %123, <4 x i32>* %124, align 4, !tbaa !133
  %125 = add nuw nsw i64 %7, 61
  %126 = add nsw i64 %16, 581
  %127 = getelementptr inbounds float, float* %6, i64 %126
  %128 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %125
  %129 = bitcast float* %127 to <4 x i32>*
  %130 = load <4 x i32>, <4 x i32>* %129, align 4, !tbaa !136
  %131 = bitcast float* %128 to <4 x i32>*
  store <4 x i32> %130, <4 x i32>* %131, align 4, !tbaa !133
  %132 = add nuw nsw i64 %7, 65
  %133 = add nsw i64 %16, 585
  %134 = getelementptr inbounds float, float* %6, i64 %133
  %135 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %132
  %136 = bitcast float* %134 to <4 x i32>*
  %137 = load <4 x i32>, <4 x i32>* %136, align 4, !tbaa !136
  %138 = bitcast float* %135 to <4 x i32>*
  store <4 x i32> %137, <4 x i32>* %138, align 4, !tbaa !133
  %139 = add nuw nsw i64 %7, 69
  %140 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %139
  store float 0.000000e+00, float* %140, align 4, !tbaa !133
  %141 = add nuw nsw i64 %7, 70
  %142 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %141
  store float 0.000000e+00, float* %142, align 8, !tbaa !133
  %143 = or i64 %141, 1
  %144 = add nsw i64 %16, 721
  %145 = getelementptr inbounds float, float* %6, i64 %144
  %146 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %143
  %147 = bitcast float* %145 to <4 x i32>*
  %148 = load <4 x i32>, <4 x i32>* %147, align 4, !tbaa !136
  %149 = bitcast float* %146 to <4 x i32>*
  store <4 x i32> %148, <4 x i32>* %149, align 4, !tbaa !133
  %150 = add nuw nsw i64 %7, 75
  %151 = add nsw i64 %16, 725
  %152 = getelementptr inbounds float, float* %6, i64 %151
  %153 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %150
  %154 = bitcast float* %152 to <4 x i32>*
  %155 = load <4 x i32>, <4 x i32>* %154, align 4, !tbaa !136
  %156 = bitcast float* %153 to <4 x i32>*
  store <4 x i32> %155, <4 x i32>* %156, align 4, !tbaa !133
  %157 = add nuw nsw i64 %7, 79
  %158 = add nsw i64 %16, 729
  %159 = getelementptr inbounds float, float* %6, i64 %158
  %160 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %157
  %161 = bitcast float* %159 to <4 x i32>*
  %162 = load <4 x i32>, <4 x i32>* %161, align 4, !tbaa !136
  %163 = bitcast float* %160 to <4 x i32>*
  store <4 x i32> %162, <4 x i32>* %163, align 4, !tbaa !133
  %164 = add nuw nsw i64 %7, 83
  %165 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %164
  store float 0.000000e+00, float* %165, align 4, !tbaa !133
  %166 = add nuw nsw i64 %7, 84
  %167 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %166
  store float 0.000000e+00, float* %167, align 16, !tbaa !133
  %168 = or i64 %166, 1
  %169 = add nsw i64 %16, 865
  %170 = getelementptr inbounds float, float* %6, i64 %169
  %171 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %168
  %172 = bitcast float* %170 to <4 x i32>*
  %173 = load <4 x i32>, <4 x i32>* %172, align 4, !tbaa !136
  %174 = bitcast float* %171 to <4 x i32>*
  store <4 x i32> %173, <4 x i32>* %174, align 4, !tbaa !133
  %175 = add nuw nsw i64 %7, 89
  %176 = add nsw i64 %16, 869
  %177 = getelementptr inbounds float, float* %6, i64 %176
  %178 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %175
  %179 = bitcast float* %177 to <4 x i32>*
  %180 = load <4 x i32>, <4 x i32>* %179, align 4, !tbaa !136
  %181 = bitcast float* %178 to <4 x i32>*
  store <4 x i32> %180, <4 x i32>* %181, align 4, !tbaa !133
  %182 = add nuw nsw i64 %7, 93
  %183 = add nsw i64 %16, 873
  %184 = getelementptr inbounds float, float* %6, i64 %183
  %185 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %182
  %186 = bitcast float* %184 to <4 x i32>*
  %187 = load <4 x i32>, <4 x i32>* %186, align 4, !tbaa !136
  %188 = bitcast float* %185 to <4 x i32>*
  store <4 x i32> %187, <4 x i32>* %188, align 4, !tbaa !133
  %189 = add nuw nsw i64 %7, 97
  %190 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %189
  store float 0.000000e+00, float* %190, align 4, !tbaa !133
  %191 = add nuw nsw i64 %7, 98
  %192 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %191
  store float 0.000000e+00, float* %192, align 8, !tbaa !133
  %193 = or i64 %191, 1
  %194 = add nsw i64 %16, 1009
  %195 = getelementptr inbounds float, float* %6, i64 %194
  %196 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %193
  %197 = bitcast float* %195 to <4 x i32>*
  %198 = load <4 x i32>, <4 x i32>* %197, align 4, !tbaa !136
  %199 = bitcast float* %196 to <4 x i32>*
  store <4 x i32> %198, <4 x i32>* %199, align 4, !tbaa !133
  %200 = add nuw nsw i64 %7, 103
  %201 = add nsw i64 %16, 1013
  %202 = getelementptr inbounds float, float* %6, i64 %201
  %203 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %200
  %204 = bitcast float* %202 to <4 x i32>*
  %205 = load <4 x i32>, <4 x i32>* %204, align 4, !tbaa !136
  %206 = bitcast float* %203 to <4 x i32>*
  store <4 x i32> %205, <4 x i32>* %206, align 4, !tbaa !133
  %207 = add nuw nsw i64 %7, 107
  %208 = add nsw i64 %16, 1017
  %209 = getelementptr inbounds float, float* %6, i64 %208
  %210 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %207
  %211 = bitcast float* %209 to <4 x i32>*
  %212 = load <4 x i32>, <4 x i32>* %211, align 4, !tbaa !136
  %213 = bitcast float* %210 to <4 x i32>*
  store <4 x i32> %212, <4 x i32>* %213, align 4, !tbaa !133
  %214 = add nuw nsw i64 %7, 111
  %215 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %214
  store float 0.000000e+00, float* %215, align 4, !tbaa !133
  br label %for_end3

for_begin7.preheader:                             ; preds = %for_end3
  %.sub = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0
  %216 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_end3:                                         ; preds = %for_begin4.preheader.us.preheader, %for_begin4.preheader.preheader
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond150 = icmp eq i64 %indvar.next, 896
  br i1 %exitcond150, label %for_begin7.preheader, label %for_begin1.preheader, !prof !28

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %217 = phi i32 [ 0, %for_begin7.preheader ], [ %255, %for_end12 ]
  %218 = urem i32 %217, 3
  %219 = mul nuw nsw i32 %218, 192
  %220 = udiv i32 %217, 3
  %221 = mul nsw i32 %220, 36864
  %222 = or i32 %219, %221
  %223 = mul nuw nsw i32 %218, 3
  %224 = or i32 %223, %221
  %225 = zext i32 %224 to i64
  %226 = zext i32 %222 to i64
  br label %for_begin13.preheader

for_begin19.preheader:                            ; preds = %for_end12
  %227 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 8
  %228 = bitcast float* %227 to <8 x float>*
  %229 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 16
  %230 = bitcast float* %229 to <8 x float>*
  %231 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 24
  %232 = bitcast float* %231 to <8 x float>*
  %233 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 32
  %234 = bitcast float* %233 to <8 x float>*
  %235 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 40
  %236 = bitcast float* %235 to <8 x float>*
  %237 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 48
  %238 = bitcast float* %237 to <8 x float>*
  %239 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 56
  %240 = bitcast float* %239 to <8 x float>*
  %241 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 64
  %242 = bitcast float* %241 to <8 x float>*
  %243 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 72
  %244 = bitcast float* %243 to <8 x float>*
  %245 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 80
  %246 = bitcast float* %245 to <8 x float>*
  %247 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 88
  %248 = bitcast float* %247 to <8 x float>*
  %249 = bitcast i8* %2 to float*
  %250 = bitcast [12 x <8 x float>]* %3 to i8*
  br label %for_body20

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvars.iv131 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next132, %for_end15 ]
  %251 = mul nuw nsw i64 %indvars.iv131, 576
  %252 = add nuw nsw i64 %251, %226
  %253 = mul nuw nsw i64 %indvars.iv131, 72
  %254 = add nuw nsw i64 %253, %225
  br label %for_begin16.preheader

for_end12:                                        ; preds = %for_end15
  %255 = add nuw nsw i32 %217, 1
  %exitcond134 = icmp eq i32 %255, 192
  br i1 %exitcond134, label %for_begin19.preheader, label %for_begin10.preheader, !prof !28

for_begin16.preheader:                            ; preds = %for_begin16.preheader, %for_begin13.preheader
  %indvars.iv128 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next129, %for_begin16.preheader ]
  %256 = shl i64 %indvars.iv128, 6
  %257 = add nuw nsw i64 %252, %256
  %258 = add nuw nsw i64 %254, %indvars.iv128
  %259 = getelementptr inbounds float, float* %216, i64 %258
  %260 = load float, float* %259, align 4, !tbaa !139
  %261 = insertelement <8 x float> undef, float %260, i32 0
  %262 = shl i64 %258, 32
  %sext = add i64 %262, 19791209299968
  %263 = ashr exact i64 %sext, 32
  %264 = getelementptr inbounds float, float* %216, i64 %263
  %265 = load float, float* %264, align 4, !tbaa !139
  %266 = insertelement <8 x float> %261, float %265, i32 1
  %267 = shl i64 %258, 32
  %sext151 = add i64 %267, 39582418599936
  %268 = ashr exact i64 %sext151, 32
  %269 = getelementptr inbounds float, float* %216, i64 %268
  %270 = load float, float* %269, align 4, !tbaa !139
  %271 = insertelement <8 x float> %266, float %270, i32 2
  %272 = shl i64 %258, 32
  %sext152 = add i64 %272, 59373627899904
  %273 = ashr exact i64 %sext152, 32
  %274 = getelementptr inbounds float, float* %216, i64 %273
  %275 = load float, float* %274, align 4, !tbaa !139
  %276 = insertelement <8 x float> %271, float %275, i32 3
  %277 = shl i64 %258, 32
  %sext153 = add i64 %277, 79164837199872
  %278 = ashr exact i64 %sext153, 32
  %279 = getelementptr inbounds float, float* %216, i64 %278
  %280 = load float, float* %279, align 4, !tbaa !139
  %281 = insertelement <8 x float> %276, float %280, i32 4
  %282 = shl i64 %258, 32
  %sext154 = add i64 %282, 98956046499840
  %283 = ashr exact i64 %sext154, 32
  %284 = getelementptr inbounds float, float* %216, i64 %283
  %285 = load float, float* %284, align 4, !tbaa !139
  %286 = insertelement <8 x float> %281, float %285, i32 5
  %287 = shl i64 %258, 32
  %sext155 = add i64 %287, 118747255799808
  %288 = ashr exact i64 %sext155, 32
  %289 = getelementptr inbounds float, float* %216, i64 %288
  %290 = load float, float* %289, align 4, !tbaa !139
  %291 = insertelement <8 x float> %286, float %290, i32 6
  %292 = shl i64 %258, 32
  %sext156 = add i64 %292, 138538465099776
  %293 = ashr exact i64 %sext156, 32
  %294 = getelementptr inbounds float, float* %216, i64 %293
  %295 = load float, float* %294, align 4, !tbaa !139
  %296 = insertelement <8 x float> %291, float %295, i32 7
  %297 = getelementptr inbounds [294912 x <8 x float>], [294912 x <8 x float>]* %4, i64 0, i64 0, i64 %257
  %298 = bitcast float* %297 to <8 x float>*
  store <8 x float> %296, <8 x float>* %298, align 16, !tbaa !142
  %299 = or i64 %257, 8
  %300 = add nuw nsw i64 %258, 9
  %301 = getelementptr inbounds float, float* %216, i64 %300
  %302 = load float, float* %301, align 4, !tbaa !139
  %303 = insertelement <8 x float> undef, float %302, i32 0
  %304 = shl i64 %258, 32
  %sext157 = add i64 %304, 19829864005632
  %305 = ashr exact i64 %sext157, 32
  %306 = getelementptr inbounds float, float* %216, i64 %305
  %307 = load float, float* %306, align 4, !tbaa !139
  %308 = insertelement <8 x float> %303, float %307, i32 1
  %309 = shl i64 %258, 32
  %sext158 = add i64 %309, 39621073305600
  %310 = ashr exact i64 %sext158, 32
  %311 = getelementptr inbounds float, float* %216, i64 %310
  %312 = load float, float* %311, align 4, !tbaa !139
  %313 = insertelement <8 x float> %308, float %312, i32 2
  %314 = shl i64 %258, 32
  %sext159 = add i64 %314, 59412282605568
  %315 = ashr exact i64 %sext159, 32
  %316 = getelementptr inbounds float, float* %216, i64 %315
  %317 = load float, float* %316, align 4, !tbaa !139
  %318 = insertelement <8 x float> %313, float %317, i32 3
  %319 = shl i64 %258, 32
  %sext160 = add i64 %319, 79203491905536
  %320 = ashr exact i64 %sext160, 32
  %321 = getelementptr inbounds float, float* %216, i64 %320
  %322 = load float, float* %321, align 4, !tbaa !139
  %323 = insertelement <8 x float> %318, float %322, i32 4
  %324 = shl i64 %258, 32
  %sext161 = add i64 %324, 98994701205504
  %325 = ashr exact i64 %sext161, 32
  %326 = getelementptr inbounds float, float* %216, i64 %325
  %327 = load float, float* %326, align 4, !tbaa !139
  %328 = insertelement <8 x float> %323, float %327, i32 5
  %329 = shl i64 %258, 32
  %sext162 = add i64 %329, 118785910505472
  %330 = ashr exact i64 %sext162, 32
  %331 = getelementptr inbounds float, float* %216, i64 %330
  %332 = load float, float* %331, align 4, !tbaa !139
  %333 = insertelement <8 x float> %328, float %332, i32 6
  %334 = shl i64 %258, 32
  %sext163 = add i64 %334, 138577119805440
  %335 = ashr exact i64 %sext163, 32
  %336 = getelementptr inbounds float, float* %216, i64 %335
  %337 = load float, float* %336, align 4, !tbaa !139
  %338 = insertelement <8 x float> %333, float %337, i32 7
  %339 = getelementptr inbounds [294912 x <8 x float>], [294912 x <8 x float>]* %4, i64 0, i64 0, i64 %299
  %340 = bitcast float* %339 to <8 x float>*
  store <8 x float> %338, <8 x float>* %340, align 16, !tbaa !142
  %341 = or i64 %257, 16
  %342 = add nuw nsw i64 %258, 18
  %343 = getelementptr inbounds float, float* %216, i64 %342
  %344 = load float, float* %343, align 4, !tbaa !139
  %345 = insertelement <8 x float> undef, float %344, i32 0
  %346 = shl i64 %258, 32
  %sext164 = add i64 %346, 19868518711296
  %347 = ashr exact i64 %sext164, 32
  %348 = getelementptr inbounds float, float* %216, i64 %347
  %349 = load float, float* %348, align 4, !tbaa !139
  %350 = insertelement <8 x float> %345, float %349, i32 1
  %351 = shl i64 %258, 32
  %sext165 = add i64 %351, 39659728011264
  %352 = ashr exact i64 %sext165, 32
  %353 = getelementptr inbounds float, float* %216, i64 %352
  %354 = load float, float* %353, align 4, !tbaa !139
  %355 = insertelement <8 x float> %350, float %354, i32 2
  %356 = shl i64 %258, 32
  %sext166 = add i64 %356, 59450937311232
  %357 = ashr exact i64 %sext166, 32
  %358 = getelementptr inbounds float, float* %216, i64 %357
  %359 = load float, float* %358, align 4, !tbaa !139
  %360 = insertelement <8 x float> %355, float %359, i32 3
  %361 = shl i64 %258, 32
  %sext167 = add i64 %361, 79242146611200
  %362 = ashr exact i64 %sext167, 32
  %363 = getelementptr inbounds float, float* %216, i64 %362
  %364 = load float, float* %363, align 4, !tbaa !139
  %365 = insertelement <8 x float> %360, float %364, i32 4
  %366 = shl i64 %258, 32
  %sext168 = add i64 %366, 99033355911168
  %367 = ashr exact i64 %sext168, 32
  %368 = getelementptr inbounds float, float* %216, i64 %367
  %369 = load float, float* %368, align 4, !tbaa !139
  %370 = insertelement <8 x float> %365, float %369, i32 5
  %371 = shl i64 %258, 32
  %sext169 = add i64 %371, 118824565211136
  %372 = ashr exact i64 %sext169, 32
  %373 = getelementptr inbounds float, float* %216, i64 %372
  %374 = load float, float* %373, align 4, !tbaa !139
  %375 = insertelement <8 x float> %370, float %374, i32 6
  %376 = shl i64 %258, 32
  %sext170 = add i64 %376, 138615774511104
  %377 = ashr exact i64 %sext170, 32
  %378 = getelementptr inbounds float, float* %216, i64 %377
  %379 = load float, float* %378, align 4, !tbaa !139
  %380 = insertelement <8 x float> %375, float %379, i32 7
  %381 = getelementptr inbounds [294912 x <8 x float>], [294912 x <8 x float>]* %4, i64 0, i64 0, i64 %341
  %382 = bitcast float* %381 to <8 x float>*
  store <8 x float> %380, <8 x float>* %382, align 16, !tbaa !142
  %383 = or i64 %257, 24
  %384 = add nuw nsw i64 %258, 27
  %385 = getelementptr inbounds float, float* %216, i64 %384
  %386 = load float, float* %385, align 4, !tbaa !139
  %387 = insertelement <8 x float> undef, float %386, i32 0
  %388 = shl i64 %258, 32
  %sext171 = add i64 %388, 19907173416960
  %389 = ashr exact i64 %sext171, 32
  %390 = getelementptr inbounds float, float* %216, i64 %389
  %391 = load float, float* %390, align 4, !tbaa !139
  %392 = insertelement <8 x float> %387, float %391, i32 1
  %393 = shl i64 %258, 32
  %sext172 = add i64 %393, 39698382716928
  %394 = ashr exact i64 %sext172, 32
  %395 = getelementptr inbounds float, float* %216, i64 %394
  %396 = load float, float* %395, align 4, !tbaa !139
  %397 = insertelement <8 x float> %392, float %396, i32 2
  %398 = shl i64 %258, 32
  %sext173 = add i64 %398, 59489592016896
  %399 = ashr exact i64 %sext173, 32
  %400 = getelementptr inbounds float, float* %216, i64 %399
  %401 = load float, float* %400, align 4, !tbaa !139
  %402 = insertelement <8 x float> %397, float %401, i32 3
  %403 = shl i64 %258, 32
  %sext174 = add i64 %403, 79280801316864
  %404 = ashr exact i64 %sext174, 32
  %405 = getelementptr inbounds float, float* %216, i64 %404
  %406 = load float, float* %405, align 4, !tbaa !139
  %407 = insertelement <8 x float> %402, float %406, i32 4
  %408 = shl i64 %258, 32
  %sext175 = add i64 %408, 99072010616832
  %409 = ashr exact i64 %sext175, 32
  %410 = getelementptr inbounds float, float* %216, i64 %409
  %411 = load float, float* %410, align 4, !tbaa !139
  %412 = insertelement <8 x float> %407, float %411, i32 5
  %413 = shl i64 %258, 32
  %sext176 = add i64 %413, 118863219916800
  %414 = ashr exact i64 %sext176, 32
  %415 = getelementptr inbounds float, float* %216, i64 %414
  %416 = load float, float* %415, align 4, !tbaa !139
  %417 = insertelement <8 x float> %412, float %416, i32 6
  %418 = shl i64 %258, 32
  %sext177 = add i64 %418, 138654429216768
  %419 = ashr exact i64 %sext177, 32
  %420 = getelementptr inbounds float, float* %216, i64 %419
  %421 = load float, float* %420, align 4, !tbaa !139
  %422 = insertelement <8 x float> %417, float %421, i32 7
  %423 = getelementptr inbounds [294912 x <8 x float>], [294912 x <8 x float>]* %4, i64 0, i64 0, i64 %383
  %424 = bitcast float* %423 to <8 x float>*
  store <8 x float> %422, <8 x float>* %424, align 16, !tbaa !142
  %425 = or i64 %257, 32
  %426 = add nuw nsw i64 %258, 36
  %427 = getelementptr inbounds float, float* %216, i64 %426
  %428 = load float, float* %427, align 4, !tbaa !139
  %429 = insertelement <8 x float> undef, float %428, i32 0
  %430 = shl i64 %258, 32
  %sext178 = add i64 %430, 19945828122624
  %431 = ashr exact i64 %sext178, 32
  %432 = getelementptr inbounds float, float* %216, i64 %431
  %433 = load float, float* %432, align 4, !tbaa !139
  %434 = insertelement <8 x float> %429, float %433, i32 1
  %435 = shl i64 %258, 32
  %sext179 = add i64 %435, 39737037422592
  %436 = ashr exact i64 %sext179, 32
  %437 = getelementptr inbounds float, float* %216, i64 %436
  %438 = load float, float* %437, align 4, !tbaa !139
  %439 = insertelement <8 x float> %434, float %438, i32 2
  %440 = shl i64 %258, 32
  %sext180 = add i64 %440, 59528246722560
  %441 = ashr exact i64 %sext180, 32
  %442 = getelementptr inbounds float, float* %216, i64 %441
  %443 = load float, float* %442, align 4, !tbaa !139
  %444 = insertelement <8 x float> %439, float %443, i32 3
  %445 = shl i64 %258, 32
  %sext181 = add i64 %445, 79319456022528
  %446 = ashr exact i64 %sext181, 32
  %447 = getelementptr inbounds float, float* %216, i64 %446
  %448 = load float, float* %447, align 4, !tbaa !139
  %449 = insertelement <8 x float> %444, float %448, i32 4
  %450 = shl i64 %258, 32
  %sext182 = add i64 %450, 99110665322496
  %451 = ashr exact i64 %sext182, 32
  %452 = getelementptr inbounds float, float* %216, i64 %451
  %453 = load float, float* %452, align 4, !tbaa !139
  %454 = insertelement <8 x float> %449, float %453, i32 5
  %455 = shl i64 %258, 32
  %sext183 = add i64 %455, 118901874622464
  %456 = ashr exact i64 %sext183, 32
  %457 = getelementptr inbounds float, float* %216, i64 %456
  %458 = load float, float* %457, align 4, !tbaa !139
  %459 = insertelement <8 x float> %454, float %458, i32 6
  %460 = shl i64 %258, 32
  %sext184 = add i64 %460, 138693083922432
  %461 = ashr exact i64 %sext184, 32
  %462 = getelementptr inbounds float, float* %216, i64 %461
  %463 = load float, float* %462, align 4, !tbaa !139
  %464 = insertelement <8 x float> %459, float %463, i32 7
  %465 = getelementptr inbounds [294912 x <8 x float>], [294912 x <8 x float>]* %4, i64 0, i64 0, i64 %425
  %466 = bitcast float* %465 to <8 x float>*
  store <8 x float> %464, <8 x float>* %466, align 16, !tbaa !142
  %467 = or i64 %257, 40
  %468 = add nuw nsw i64 %258, 45
  %469 = getelementptr inbounds float, float* %216, i64 %468
  %470 = load float, float* %469, align 4, !tbaa !139
  %471 = insertelement <8 x float> undef, float %470, i32 0
  %472 = shl i64 %258, 32
  %sext185 = add i64 %472, 19984482828288
  %473 = ashr exact i64 %sext185, 32
  %474 = getelementptr inbounds float, float* %216, i64 %473
  %475 = load float, float* %474, align 4, !tbaa !139
  %476 = insertelement <8 x float> %471, float %475, i32 1
  %477 = shl i64 %258, 32
  %sext186 = add i64 %477, 39775692128256
  %478 = ashr exact i64 %sext186, 32
  %479 = getelementptr inbounds float, float* %216, i64 %478
  %480 = load float, float* %479, align 4, !tbaa !139
  %481 = insertelement <8 x float> %476, float %480, i32 2
  %482 = shl i64 %258, 32
  %sext187 = add i64 %482, 59566901428224
  %483 = ashr exact i64 %sext187, 32
  %484 = getelementptr inbounds float, float* %216, i64 %483
  %485 = load float, float* %484, align 4, !tbaa !139
  %486 = insertelement <8 x float> %481, float %485, i32 3
  %487 = shl i64 %258, 32
  %sext188 = add i64 %487, 79358110728192
  %488 = ashr exact i64 %sext188, 32
  %489 = getelementptr inbounds float, float* %216, i64 %488
  %490 = load float, float* %489, align 4, !tbaa !139
  %491 = insertelement <8 x float> %486, float %490, i32 4
  %492 = shl i64 %258, 32
  %sext189 = add i64 %492, 99149320028160
  %493 = ashr exact i64 %sext189, 32
  %494 = getelementptr inbounds float, float* %216, i64 %493
  %495 = load float, float* %494, align 4, !tbaa !139
  %496 = insertelement <8 x float> %491, float %495, i32 5
  %497 = shl i64 %258, 32
  %sext190 = add i64 %497, 118940529328128
  %498 = ashr exact i64 %sext190, 32
  %499 = getelementptr inbounds float, float* %216, i64 %498
  %500 = load float, float* %499, align 4, !tbaa !139
  %501 = insertelement <8 x float> %496, float %500, i32 6
  %502 = shl i64 %258, 32
  %sext191 = add i64 %502, 138731738628096
  %503 = ashr exact i64 %sext191, 32
  %504 = getelementptr inbounds float, float* %216, i64 %503
  %505 = load float, float* %504, align 4, !tbaa !139
  %506 = insertelement <8 x float> %501, float %505, i32 7
  %507 = getelementptr inbounds [294912 x <8 x float>], [294912 x <8 x float>]* %4, i64 0, i64 0, i64 %467
  %508 = bitcast float* %507 to <8 x float>*
  store <8 x float> %506, <8 x float>* %508, align 16, !tbaa !142
  %509 = or i64 %257, 48
  %510 = add nuw nsw i64 %258, 54
  %511 = getelementptr inbounds float, float* %216, i64 %510
  %512 = load float, float* %511, align 4, !tbaa !139
  %513 = insertelement <8 x float> undef, float %512, i32 0
  %514 = shl i64 %258, 32
  %sext192 = add i64 %514, 20023137533952
  %515 = ashr exact i64 %sext192, 32
  %516 = getelementptr inbounds float, float* %216, i64 %515
  %517 = load float, float* %516, align 4, !tbaa !139
  %518 = insertelement <8 x float> %513, float %517, i32 1
  %519 = shl i64 %258, 32
  %sext193 = add i64 %519, 39814346833920
  %520 = ashr exact i64 %sext193, 32
  %521 = getelementptr inbounds float, float* %216, i64 %520
  %522 = load float, float* %521, align 4, !tbaa !139
  %523 = insertelement <8 x float> %518, float %522, i32 2
  %524 = shl i64 %258, 32
  %sext194 = add i64 %524, 59605556133888
  %525 = ashr exact i64 %sext194, 32
  %526 = getelementptr inbounds float, float* %216, i64 %525
  %527 = load float, float* %526, align 4, !tbaa !139
  %528 = insertelement <8 x float> %523, float %527, i32 3
  %529 = shl i64 %258, 32
  %sext195 = add i64 %529, 79396765433856
  %530 = ashr exact i64 %sext195, 32
  %531 = getelementptr inbounds float, float* %216, i64 %530
  %532 = load float, float* %531, align 4, !tbaa !139
  %533 = insertelement <8 x float> %528, float %532, i32 4
  %534 = shl i64 %258, 32
  %sext196 = add i64 %534, 99187974733824
  %535 = ashr exact i64 %sext196, 32
  %536 = getelementptr inbounds float, float* %216, i64 %535
  %537 = load float, float* %536, align 4, !tbaa !139
  %538 = insertelement <8 x float> %533, float %537, i32 5
  %539 = shl i64 %258, 32
  %sext197 = add i64 %539, 118979184033792
  %540 = ashr exact i64 %sext197, 32
  %541 = getelementptr inbounds float, float* %216, i64 %540
  %542 = load float, float* %541, align 4, !tbaa !139
  %543 = insertelement <8 x float> %538, float %542, i32 6
  %544 = shl i64 %258, 32
  %sext198 = add i64 %544, 138770393333760
  %545 = ashr exact i64 %sext198, 32
  %546 = getelementptr inbounds float, float* %216, i64 %545
  %547 = load float, float* %546, align 4, !tbaa !139
  %548 = insertelement <8 x float> %543, float %547, i32 7
  %549 = getelementptr inbounds [294912 x <8 x float>], [294912 x <8 x float>]* %4, i64 0, i64 0, i64 %509
  %550 = bitcast float* %549 to <8 x float>*
  store <8 x float> %548, <8 x float>* %550, align 16, !tbaa !142
  %551 = or i64 %257, 56
  %552 = add nuw nsw i64 %258, 63
  %553 = getelementptr inbounds float, float* %216, i64 %552
  %554 = load float, float* %553, align 4, !tbaa !139
  %555 = insertelement <8 x float> undef, float %554, i32 0
  %556 = shl i64 %258, 32
  %sext199 = add i64 %556, 20061792239616
  %557 = ashr exact i64 %sext199, 32
  %558 = getelementptr inbounds float, float* %216, i64 %557
  %559 = load float, float* %558, align 4, !tbaa !139
  %560 = insertelement <8 x float> %555, float %559, i32 1
  %561 = shl i64 %258, 32
  %sext200 = add i64 %561, 39853001539584
  %562 = ashr exact i64 %sext200, 32
  %563 = getelementptr inbounds float, float* %216, i64 %562
  %564 = load float, float* %563, align 4, !tbaa !139
  %565 = insertelement <8 x float> %560, float %564, i32 2
  %566 = shl i64 %258, 32
  %sext201 = add i64 %566, 59644210839552
  %567 = ashr exact i64 %sext201, 32
  %568 = getelementptr inbounds float, float* %216, i64 %567
  %569 = load float, float* %568, align 4, !tbaa !139
  %570 = insertelement <8 x float> %565, float %569, i32 3
  %571 = shl i64 %258, 32
  %sext202 = add i64 %571, 79435420139520
  %572 = ashr exact i64 %sext202, 32
  %573 = getelementptr inbounds float, float* %216, i64 %572
  %574 = load float, float* %573, align 4, !tbaa !139
  %575 = insertelement <8 x float> %570, float %574, i32 4
  %576 = shl i64 %258, 32
  %sext203 = add i64 %576, 99226629439488
  %577 = ashr exact i64 %sext203, 32
  %578 = getelementptr inbounds float, float* %216, i64 %577
  %579 = load float, float* %578, align 4, !tbaa !139
  %580 = insertelement <8 x float> %575, float %579, i32 5
  %581 = shl i64 %258, 32
  %sext204 = add i64 %581, 119017838739456
  %582 = ashr exact i64 %sext204, 32
  %583 = getelementptr inbounds float, float* %216, i64 %582
  %584 = load float, float* %583, align 4, !tbaa !139
  %585 = insertelement <8 x float> %580, float %584, i32 6
  %586 = shl i64 %258, 32
  %sext205 = add i64 %586, 138809048039424
  %587 = ashr exact i64 %sext205, 32
  %588 = getelementptr inbounds float, float* %216, i64 %587
  %589 = load float, float* %588, align 4, !tbaa !139
  %590 = insertelement <8 x float> %585, float %589, i32 7
  %591 = getelementptr inbounds [294912 x <8 x float>], [294912 x <8 x float>]* %4, i64 0, i64 0, i64 %551
  %592 = bitcast float* %591 to <8 x float>*
  store <8 x float> %590, <8 x float>* %592, align 16, !tbaa !142
  %indvars.iv.next129 = add nuw nsw i64 %indvars.iv128, 1
  %exitcond130 = icmp eq i64 %indvars.iv.next129, 3
  br i1 %exitcond130, label %for_end15, label %for_begin16.preheader, !prof !28

for_end15:                                        ; preds = %for_begin16.preheader
  %indvars.iv.next132 = add nuw nsw i64 %indvars.iv131, 1
  %exitcond133 = icmp eq i64 %indvars.iv.next132, 64
  br i1 %exitcond133, label %for_end12, label %for_begin13.preheader, !prof !28

for_body20:                                       ; preds = %for_end36, %for_begin19.preheader
  %593 = phi i32 [ 0, %for_begin19.preheader ], [ %732, %for_end36 ]
  %594 = urem i32 %593, 12
  %595 = udiv i32 %593, 12
  %596 = mul nsw i32 %595, 36864
  %597 = zext i32 %596 to i64
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %250, i8 0, i64 384, i1 false)
  br label %for_begin25.preheader

for_end21:                                        ; preds = %for_end36
  ret void

for_begin34.preheader:                            ; preds = %for_end27
  store <8 x float> %850, <8 x float>* %.sub, align 16, !tbaa !145
  store <8 x float> %856, <8 x float>* %228, align 16, !tbaa !145
  store <8 x float> %862, <8 x float>* %230, align 16, !tbaa !145
  store <8 x float> %868, <8 x float>* %232, align 16, !tbaa !145
  store <8 x float> %874, <8 x float>* %234, align 16, !tbaa !145
  store <8 x float> %880, <8 x float>* %236, align 16, !tbaa !145
  store <8 x float> %886, <8 x float>* %238, align 16, !tbaa !145
  store <8 x float> %892, <8 x float>* %240, align 16, !tbaa !145
  store <8 x float> %898, <8 x float>* %242, align 16, !tbaa !145
  store <8 x float> %904, <8 x float>* %244, align 16, !tbaa !145
  store <8 x float> %910, <8 x float>* %246, align 16, !tbaa !145
  store <8 x float> %916, <8 x float>* %248, align 16, !tbaa !145
  %598 = mul nuw nsw i32 %594, 12
  %599 = mul nsw i32 %595, 1152
  %600 = add nuw nsw i32 %599, %598
  %601 = zext i32 %600 to i64
  br label %for_body35

for_begin25.preheader:                            ; preds = %for_end27, %for_body20
  %indvars.iv118 = phi i64 [ 0, %for_body20 ], [ %indvars.iv.next119, %for_end27 ]
  %.lcssa23.lcssa.lcssa91 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %916, %for_end27 ]
  %.lcssa21.lcssa.lcssa89 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %910, %for_end27 ]
  %.lcssa19.lcssa.lcssa87 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %904, %for_end27 ]
  %.lcssa17.lcssa.lcssa85 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %898, %for_end27 ]
  %.lcssa15.lcssa.lcssa83 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %892, %for_end27 ]
  %.lcssa13.lcssa.lcssa81 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %886, %for_end27 ]
  %.lcssa11.lcssa.lcssa79 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %880, %for_end27 ]
  %.lcssa9.lcssa.lcssa77 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %874, %for_end27 ]
  %.lcssa7.lcssa.lcssa76 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %868, %for_end27 ]
  %.lcssa5.lcssa.lcssa74 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %862, %for_end27 ]
  %.lcssa3.lcssa.lcssa72 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %856, %for_end27 ]
  %.lcssa.lcssa.lcssa70 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %850, %for_end27 ]
  %602 = mul nuw nsw i64 %indvars.iv118, 576
  %603 = add nuw nsw i64 %602, %597
  %604 = trunc i64 %indvars.iv118 to i32
  %605 = mul i32 %604, 1568
  br label %for_begin28.preheader

for_begin28.preheader:                            ; preds = %for_end33.2, %for_begin25.preheader
  %indvars.iv114 = phi i64 [ 0, %for_begin25.preheader ], [ %indvars.iv.next115, %for_end33.2 ]
  %.lcssa23.lcssa68 = phi <8 x float> [ %.lcssa23.lcssa.lcssa91, %for_begin25.preheader ], [ %916, %for_end33.2 ]
  %.lcssa21.lcssa66 = phi <8 x float> [ %.lcssa21.lcssa.lcssa89, %for_begin25.preheader ], [ %910, %for_end33.2 ]
  %.lcssa19.lcssa64 = phi <8 x float> [ %.lcssa19.lcssa.lcssa87, %for_begin25.preheader ], [ %904, %for_end33.2 ]
  %.lcssa17.lcssa62 = phi <8 x float> [ %.lcssa17.lcssa.lcssa85, %for_begin25.preheader ], [ %898, %for_end33.2 ]
  %.lcssa15.lcssa60 = phi <8 x float> [ %.lcssa15.lcssa.lcssa83, %for_begin25.preheader ], [ %892, %for_end33.2 ]
  %.lcssa13.lcssa58 = phi <8 x float> [ %.lcssa13.lcssa.lcssa81, %for_begin25.preheader ], [ %886, %for_end33.2 ]
  %.lcssa11.lcssa56 = phi <8 x float> [ %.lcssa11.lcssa.lcssa79, %for_begin25.preheader ], [ %880, %for_end33.2 ]
  %.lcssa9.lcssa54 = phi <8 x float> [ %.lcssa9.lcssa.lcssa77, %for_begin25.preheader ], [ %874, %for_end33.2 ]
  %.lcssa7.lcssa52 = phi <8 x float> [ %.lcssa7.lcssa.lcssa76, %for_begin25.preheader ], [ %868, %for_end33.2 ]
  %.lcssa5.lcssa51 = phi <8 x float> [ %.lcssa5.lcssa.lcssa74, %for_begin25.preheader ], [ %862, %for_end33.2 ]
  %.lcssa3.lcssa49 = phi <8 x float> [ %.lcssa3.lcssa.lcssa72, %for_begin25.preheader ], [ %856, %for_end33.2 ]
  %.lcssa.lcssa47 = phi <8 x float> [ %.lcssa.lcssa.lcssa70, %for_begin25.preheader ], [ %850, %for_end33.2 ]
  %606 = phi i32 [ 0, %for_begin25.preheader ], [ %917, %for_end33.2 ]
  %reass.add = add nuw nsw i32 %606, %594
  %reass.mul = mul i32 %reass.add, 112
  %607 = add nsw i32 %reass.mul, %605
  %608 = mul nuw nsw i64 %indvars.iv114, 192
  %609 = add nuw nsw i64 %603, %608
  %610 = sext i32 %607 to i64
  br label %for_body32

for_end27:                                        ; preds = %for_end33.2
  %indvars.iv.next119 = add nuw nsw i64 %indvars.iv118, 1
  %exitcond120 = icmp eq i64 %indvars.iv.next119, 64
  br i1 %exitcond120, label %for_begin34.preheader, label %for_begin25.preheader, !prof !28

for_body32:                                       ; preds = %for_body32, %for_begin28.preheader
  %indvars.iv = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next, %for_body32 ]
  %611 = phi <8 x float> [ %.lcssa23.lcssa68, %for_begin28.preheader ], [ %701, %for_body32 ]
  %612 = phi <8 x float> [ %.lcssa21.lcssa66, %for_begin28.preheader ], [ %695, %for_body32 ]
  %613 = phi <8 x float> [ %.lcssa19.lcssa64, %for_begin28.preheader ], [ %689, %for_body32 ]
  %614 = phi <8 x float> [ %.lcssa17.lcssa62, %for_begin28.preheader ], [ %683, %for_body32 ]
  %615 = phi <8 x float> [ %.lcssa15.lcssa60, %for_begin28.preheader ], [ %677, %for_body32 ]
  %616 = phi <8 x float> [ %.lcssa13.lcssa58, %for_begin28.preheader ], [ %671, %for_body32 ]
  %617 = phi <8 x float> [ %.lcssa11.lcssa56, %for_begin28.preheader ], [ %665, %for_body32 ]
  %618 = phi <8 x float> [ %.lcssa9.lcssa54, %for_begin28.preheader ], [ %659, %for_body32 ]
  %619 = phi <8 x float> [ %.lcssa7.lcssa52, %for_begin28.preheader ], [ %653, %for_body32 ]
  %620 = phi <8 x float> [ %.lcssa5.lcssa51, %for_begin28.preheader ], [ %647, %for_body32 ]
  %621 = phi <8 x float> [ %.lcssa3.lcssa49, %for_begin28.preheader ], [ %641, %for_body32 ]
  %622 = phi <8 x float> [ %.lcssa.lcssa47, %for_begin28.preheader ], [ %635, %for_body32 ]
  %623 = mul nuw nsw i64 %indvars.iv, 14
  %624 = add nsw i64 %623, %610
  %625 = and i64 %624, 4294967294
  %626 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %625
  %627 = load float, float* %626, align 8, !tbaa !133
  %628 = insertelement <8 x float> undef, float %627, i32 0
  %629 = shufflevector <8 x float> %628, <8 x float> undef, <8 x i32> zeroinitializer
  %630 = shl i64 %indvars.iv, 3
  %631 = add nuw nsw i64 %609, %630
  %632 = getelementptr inbounds [294912 x <8 x float>], [294912 x <8 x float>]* %4, i64 0, i64 0, i64 %631
  %633 = bitcast float* %632 to <8 x float>*
  %634 = load <8 x float>, <8 x float>* %633, align 16, !tbaa !142
  %635 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %629, <8 x float> %634, <8 x float> %622)
  %636 = or i64 %624, 1
  %637 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %636
  %638 = load float, float* %637, align 4, !tbaa !133
  %639 = insertelement <8 x float> undef, float %638, i32 0
  %640 = shufflevector <8 x float> %639, <8 x float> undef, <8 x i32> zeroinitializer
  %641 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %640, <8 x float> %634, <8 x float> %621)
  %642 = add nuw nsw i64 %624, 2
  %643 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %642
  %644 = load float, float* %643, align 8, !tbaa !133
  %645 = insertelement <8 x float> undef, float %644, i32 0
  %646 = shufflevector <8 x float> %645, <8 x float> undef, <8 x i32> zeroinitializer
  %647 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %646, <8 x float> %634, <8 x float> %620)
  %648 = add nuw nsw i64 %624, 3
  %649 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %648
  %650 = load float, float* %649, align 4, !tbaa !133
  %651 = insertelement <8 x float> undef, float %650, i32 0
  %652 = shufflevector <8 x float> %651, <8 x float> undef, <8 x i32> zeroinitializer
  %653 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %652, <8 x float> %634, <8 x float> %619)
  %654 = add nuw nsw i64 %624, 4
  %655 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %654
  %656 = load float, float* %655, align 8, !tbaa !133
  %657 = insertelement <8 x float> undef, float %656, i32 0
  %658 = shufflevector <8 x float> %657, <8 x float> undef, <8 x i32> zeroinitializer
  %659 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %658, <8 x float> %634, <8 x float> %618)
  %660 = add nuw nsw i64 %624, 5
  %661 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %660
  %662 = load float, float* %661, align 4, !tbaa !133
  %663 = insertelement <8 x float> undef, float %662, i32 0
  %664 = shufflevector <8 x float> %663, <8 x float> undef, <8 x i32> zeroinitializer
  %665 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %664, <8 x float> %634, <8 x float> %617)
  %666 = add nuw nsw i64 %624, 6
  %667 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %666
  %668 = load float, float* %667, align 8, !tbaa !133
  %669 = insertelement <8 x float> undef, float %668, i32 0
  %670 = shufflevector <8 x float> %669, <8 x float> undef, <8 x i32> zeroinitializer
  %671 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %670, <8 x float> %634, <8 x float> %616)
  %672 = add nuw nsw i64 %624, 7
  %673 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %672
  %674 = load float, float* %673, align 4, !tbaa !133
  %675 = insertelement <8 x float> undef, float %674, i32 0
  %676 = shufflevector <8 x float> %675, <8 x float> undef, <8 x i32> zeroinitializer
  %677 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %676, <8 x float> %634, <8 x float> %615)
  %678 = add nuw nsw i64 %624, 8
  %679 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %678
  %680 = load float, float* %679, align 8, !tbaa !133
  %681 = insertelement <8 x float> undef, float %680, i32 0
  %682 = shufflevector <8 x float> %681, <8 x float> undef, <8 x i32> zeroinitializer
  %683 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %682, <8 x float> %634, <8 x float> %614)
  %684 = add nuw nsw i64 %624, 9
  %685 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %684
  %686 = load float, float* %685, align 4, !tbaa !133
  %687 = insertelement <8 x float> undef, float %686, i32 0
  %688 = shufflevector <8 x float> %687, <8 x float> undef, <8 x i32> zeroinitializer
  %689 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %688, <8 x float> %634, <8 x float> %613)
  %690 = add nuw nsw i64 %624, 10
  %691 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %690
  %692 = load float, float* %691, align 8, !tbaa !133
  %693 = insertelement <8 x float> undef, float %692, i32 0
  %694 = shufflevector <8 x float> %693, <8 x float> undef, <8 x i32> zeroinitializer
  %695 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %694, <8 x float> %634, <8 x float> %612)
  %696 = add nuw nsw i64 %624, 11
  %697 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %696
  %698 = load float, float* %697, align 4, !tbaa !133
  %699 = insertelement <8 x float> undef, float %698, i32 0
  %700 = shufflevector <8 x float> %699, <8 x float> undef, <8 x i32> zeroinitializer
  %701 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %700, <8 x float> %634, <8 x float> %611)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 8
  br i1 %exitcond, label %for_end33, label %for_body32, !prof !28

for_end33:                                        ; preds = %for_body32
  %702 = or i64 %610, 1
  %703 = add nuw nsw i64 %609, 64
  br label %for_body32.1

for_body35:                                       ; preds = %for_body35, %for_begin34.preheader
  %indvars.iv121 = phi i64 [ 0, %for_begin34.preheader ], [ %indvars.iv.next122, %for_body35 ]
  %704 = add nuw nsw i64 %indvars.iv121, %601
  %705 = add nuw nsw i64 %704, 144
  %706 = add nuw nsw i64 %704, 288
  %707 = add nuw nsw i64 %704, 432
  %708 = add nuw nsw i64 %704, 576
  %709 = add nuw nsw i64 %704, 720
  %710 = add nuw nsw i64 %704, 864
  %711 = add nuw nsw i64 %704, 1008
  %712 = shl nsw i64 %indvars.iv121, 3
  %713 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 %712
  %714 = bitcast float* %713 to <8 x float>*
  %715 = load <8 x float>, <8 x float>* %714, align 16, !tbaa !156
  %716 = getelementptr inbounds float, float* %249, i64 %704
  %717 = extractelement <8 x float> %715, i64 0
  store float %717, float* %716, align 4, !tbaa !157
  %718 = getelementptr inbounds float, float* %249, i64 %705
  %719 = extractelement <8 x float> %715, i64 1
  store float %719, float* %718, align 4, !tbaa !157
  %720 = getelementptr inbounds float, float* %249, i64 %706
  %721 = extractelement <8 x float> %715, i64 2
  store float %721, float* %720, align 4, !tbaa !157
  %722 = getelementptr inbounds float, float* %249, i64 %707
  %723 = extractelement <8 x float> %715, i64 3
  store float %723, float* %722, align 4, !tbaa !157
  %724 = getelementptr inbounds float, float* %249, i64 %708
  %725 = extractelement <8 x float> %715, i64 4
  store float %725, float* %724, align 4, !tbaa !157
  %726 = getelementptr inbounds float, float* %249, i64 %709
  %727 = extractelement <8 x float> %715, i64 5
  store float %727, float* %726, align 4, !tbaa !157
  %728 = getelementptr inbounds float, float* %249, i64 %710
  %729 = extractelement <8 x float> %715, i64 6
  store float %729, float* %728, align 4, !tbaa !157
  %730 = getelementptr inbounds float, float* %249, i64 %711
  %731 = extractelement <8 x float> %715, i64 7
  store float %731, float* %730, align 4, !tbaa !157
  %indvars.iv.next122 = add nuw nsw i64 %indvars.iv121, 1
  %exitcond123 = icmp eq i64 %indvars.iv.next122, 12
  br i1 %exitcond123, label %for_end36, label %for_body35, !prof !28

for_end36:                                        ; preds = %for_body35
  %732 = add nuw nsw i32 %593, 1
  %exitcond124 = icmp eq i32 %732, 768
  br i1 %exitcond124, label %for_end21, label %for_body20, !prof !28

for_body32.1:                                     ; preds = %for_body32.1, %for_end33
  %indvars.iv.1 = phi i64 [ 0, %for_end33 ], [ %indvars.iv.next.1, %for_body32.1 ]
  %733 = phi <8 x float> [ %701, %for_end33 ], [ %823, %for_body32.1 ]
  %734 = phi <8 x float> [ %695, %for_end33 ], [ %817, %for_body32.1 ]
  %735 = phi <8 x float> [ %689, %for_end33 ], [ %811, %for_body32.1 ]
  %736 = phi <8 x float> [ %683, %for_end33 ], [ %805, %for_body32.1 ]
  %737 = phi <8 x float> [ %677, %for_end33 ], [ %799, %for_body32.1 ]
  %738 = phi <8 x float> [ %671, %for_end33 ], [ %793, %for_body32.1 ]
  %739 = phi <8 x float> [ %665, %for_end33 ], [ %787, %for_body32.1 ]
  %740 = phi <8 x float> [ %659, %for_end33 ], [ %781, %for_body32.1 ]
  %741 = phi <8 x float> [ %653, %for_end33 ], [ %775, %for_body32.1 ]
  %742 = phi <8 x float> [ %647, %for_end33 ], [ %769, %for_body32.1 ]
  %743 = phi <8 x float> [ %641, %for_end33 ], [ %763, %for_body32.1 ]
  %744 = phi <8 x float> [ %635, %for_end33 ], [ %757, %for_body32.1 ]
  %745 = mul nuw nsw i64 %indvars.iv.1, 14
  %746 = add nsw i64 %702, %745
  %747 = and i64 %746, 4294967295
  %748 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %747
  %749 = load float, float* %748, align 4, !tbaa !133
  %750 = insertelement <8 x float> undef, float %749, i32 0
  %751 = shufflevector <8 x float> %750, <8 x float> undef, <8 x i32> zeroinitializer
  %752 = shl i64 %indvars.iv.1, 3
  %753 = add nuw nsw i64 %703, %752
  %754 = getelementptr inbounds [294912 x <8 x float>], [294912 x <8 x float>]* %4, i64 0, i64 0, i64 %753
  %755 = bitcast float* %754 to <8 x float>*
  %756 = load <8 x float>, <8 x float>* %755, align 16, !tbaa !142
  %757 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %751, <8 x float> %756, <8 x float> %744)
  %758 = add nuw nsw i64 %746, 1
  %759 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %758
  %760 = load float, float* %759, align 8, !tbaa !133
  %761 = insertelement <8 x float> undef, float %760, i32 0
  %762 = shufflevector <8 x float> %761, <8 x float> undef, <8 x i32> zeroinitializer
  %763 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %762, <8 x float> %756, <8 x float> %743)
  %764 = add nuw nsw i64 %746, 2
  %765 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %764
  %766 = load float, float* %765, align 4, !tbaa !133
  %767 = insertelement <8 x float> undef, float %766, i32 0
  %768 = shufflevector <8 x float> %767, <8 x float> undef, <8 x i32> zeroinitializer
  %769 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %768, <8 x float> %756, <8 x float> %742)
  %770 = add nuw nsw i64 %746, 3
  %771 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %770
  %772 = load float, float* %771, align 8, !tbaa !133
  %773 = insertelement <8 x float> undef, float %772, i32 0
  %774 = shufflevector <8 x float> %773, <8 x float> undef, <8 x i32> zeroinitializer
  %775 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %774, <8 x float> %756, <8 x float> %741)
  %776 = add nuw nsw i64 %746, 4
  %777 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %776
  %778 = load float, float* %777, align 4, !tbaa !133
  %779 = insertelement <8 x float> undef, float %778, i32 0
  %780 = shufflevector <8 x float> %779, <8 x float> undef, <8 x i32> zeroinitializer
  %781 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %780, <8 x float> %756, <8 x float> %740)
  %782 = add nuw nsw i64 %746, 5
  %783 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %782
  %784 = load float, float* %783, align 8, !tbaa !133
  %785 = insertelement <8 x float> undef, float %784, i32 0
  %786 = shufflevector <8 x float> %785, <8 x float> undef, <8 x i32> zeroinitializer
  %787 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %786, <8 x float> %756, <8 x float> %739)
  %788 = add nuw nsw i64 %746, 6
  %789 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %788
  %790 = load float, float* %789, align 4, !tbaa !133
  %791 = insertelement <8 x float> undef, float %790, i32 0
  %792 = shufflevector <8 x float> %791, <8 x float> undef, <8 x i32> zeroinitializer
  %793 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %792, <8 x float> %756, <8 x float> %738)
  %794 = add nuw nsw i64 %746, 7
  %795 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %794
  %796 = load float, float* %795, align 8, !tbaa !133
  %797 = insertelement <8 x float> undef, float %796, i32 0
  %798 = shufflevector <8 x float> %797, <8 x float> undef, <8 x i32> zeroinitializer
  %799 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %798, <8 x float> %756, <8 x float> %737)
  %800 = add nuw nsw i64 %746, 8
  %801 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %800
  %802 = load float, float* %801, align 4, !tbaa !133
  %803 = insertelement <8 x float> undef, float %802, i32 0
  %804 = shufflevector <8 x float> %803, <8 x float> undef, <8 x i32> zeroinitializer
  %805 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %804, <8 x float> %756, <8 x float> %736)
  %806 = add nuw nsw i64 %746, 9
  %807 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %806
  %808 = load float, float* %807, align 8, !tbaa !133
  %809 = insertelement <8 x float> undef, float %808, i32 0
  %810 = shufflevector <8 x float> %809, <8 x float> undef, <8 x i32> zeroinitializer
  %811 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %810, <8 x float> %756, <8 x float> %735)
  %812 = add nuw nsw i64 %746, 10
  %813 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %812
  %814 = load float, float* %813, align 4, !tbaa !133
  %815 = insertelement <8 x float> undef, float %814, i32 0
  %816 = shufflevector <8 x float> %815, <8 x float> undef, <8 x i32> zeroinitializer
  %817 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %816, <8 x float> %756, <8 x float> %734)
  %818 = add nuw nsw i64 %746, 11
  %819 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %818
  %820 = load float, float* %819, align 8, !tbaa !133
  %821 = insertelement <8 x float> undef, float %820, i32 0
  %822 = shufflevector <8 x float> %821, <8 x float> undef, <8 x i32> zeroinitializer
  %823 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %822, <8 x float> %756, <8 x float> %733)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 8
  br i1 %exitcond.1, label %for_end33.1, label %for_body32.1, !prof !28

for_end33.1:                                      ; preds = %for_body32.1
  %824 = or i64 %610, 2
  %825 = add nuw nsw i64 %609, 128
  br label %for_body32.2

for_body32.2:                                     ; preds = %for_body32.2, %for_end33.1
  %indvars.iv.2 = phi i64 [ 0, %for_end33.1 ], [ %indvars.iv.next.2, %for_body32.2 ]
  %826 = phi <8 x float> [ %823, %for_end33.1 ], [ %916, %for_body32.2 ]
  %827 = phi <8 x float> [ %817, %for_end33.1 ], [ %910, %for_body32.2 ]
  %828 = phi <8 x float> [ %811, %for_end33.1 ], [ %904, %for_body32.2 ]
  %829 = phi <8 x float> [ %805, %for_end33.1 ], [ %898, %for_body32.2 ]
  %830 = phi <8 x float> [ %799, %for_end33.1 ], [ %892, %for_body32.2 ]
  %831 = phi <8 x float> [ %793, %for_end33.1 ], [ %886, %for_body32.2 ]
  %832 = phi <8 x float> [ %787, %for_end33.1 ], [ %880, %for_body32.2 ]
  %833 = phi <8 x float> [ %781, %for_end33.1 ], [ %874, %for_body32.2 ]
  %834 = phi <8 x float> [ %775, %for_end33.1 ], [ %868, %for_body32.2 ]
  %835 = phi <8 x float> [ %769, %for_end33.1 ], [ %862, %for_body32.2 ]
  %836 = phi <8 x float> [ %763, %for_end33.1 ], [ %856, %for_body32.2 ]
  %837 = phi <8 x float> [ %757, %for_end33.1 ], [ %850, %for_body32.2 ]
  %838 = mul nuw nsw i64 %indvars.iv.2, 14
  %839 = add nsw i64 %824, %838
  %840 = and i64 %839, 4294967294
  %841 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %840
  %842 = load float, float* %841, align 8, !tbaa !133
  %843 = insertelement <8 x float> undef, float %842, i32 0
  %844 = shufflevector <8 x float> %843, <8 x float> undef, <8 x i32> zeroinitializer
  %845 = shl i64 %indvars.iv.2, 3
  %846 = add nuw nsw i64 %825, %845
  %847 = getelementptr inbounds [294912 x <8 x float>], [294912 x <8 x float>]* %4, i64 0, i64 0, i64 %846
  %848 = bitcast float* %847 to <8 x float>*
  %849 = load <8 x float>, <8 x float>* %848, align 16, !tbaa !142
  %850 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %844, <8 x float> %849, <8 x float> %837)
  %851 = or i64 %839, 1
  %852 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %851
  %853 = load float, float* %852, align 4, !tbaa !133
  %854 = insertelement <8 x float> undef, float %853, i32 0
  %855 = shufflevector <8 x float> %854, <8 x float> undef, <8 x i32> zeroinitializer
  %856 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %855, <8 x float> %849, <8 x float> %836)
  %857 = add nuw nsw i64 %839, 2
  %858 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %857
  %859 = load float, float* %858, align 4, !tbaa !133
  %860 = insertelement <8 x float> undef, float %859, i32 0
  %861 = shufflevector <8 x float> %860, <8 x float> undef, <8 x i32> zeroinitializer
  %862 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %861, <8 x float> %849, <8 x float> %835)
  %863 = add nuw nsw i64 %839, 3
  %864 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %863
  %865 = load float, float* %864, align 4, !tbaa !133
  %866 = insertelement <8 x float> undef, float %865, i32 0
  %867 = shufflevector <8 x float> %866, <8 x float> undef, <8 x i32> zeroinitializer
  %868 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %867, <8 x float> %849, <8 x float> %834)
  %869 = add nuw nsw i64 %839, 4
  %870 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %869
  %871 = load float, float* %870, align 4, !tbaa !133
  %872 = insertelement <8 x float> undef, float %871, i32 0
  %873 = shufflevector <8 x float> %872, <8 x float> undef, <8 x i32> zeroinitializer
  %874 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %873, <8 x float> %849, <8 x float> %833)
  %875 = add nuw nsw i64 %839, 5
  %876 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %875
  %877 = load float, float* %876, align 4, !tbaa !133
  %878 = insertelement <8 x float> undef, float %877, i32 0
  %879 = shufflevector <8 x float> %878, <8 x float> undef, <8 x i32> zeroinitializer
  %880 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %879, <8 x float> %849, <8 x float> %832)
  %881 = add nuw nsw i64 %839, 6
  %882 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %881
  %883 = load float, float* %882, align 4, !tbaa !133
  %884 = insertelement <8 x float> undef, float %883, i32 0
  %885 = shufflevector <8 x float> %884, <8 x float> undef, <8 x i32> zeroinitializer
  %886 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %885, <8 x float> %849, <8 x float> %831)
  %887 = add nuw nsw i64 %839, 7
  %888 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %887
  %889 = load float, float* %888, align 4, !tbaa !133
  %890 = insertelement <8 x float> undef, float %889, i32 0
  %891 = shufflevector <8 x float> %890, <8 x float> undef, <8 x i32> zeroinitializer
  %892 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %891, <8 x float> %849, <8 x float> %830)
  %893 = add nuw nsw i64 %839, 8
  %894 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %893
  %895 = load float, float* %894, align 4, !tbaa !133
  %896 = insertelement <8 x float> undef, float %895, i32 0
  %897 = shufflevector <8 x float> %896, <8 x float> undef, <8 x i32> zeroinitializer
  %898 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %897, <8 x float> %849, <8 x float> %829)
  %899 = add nuw nsw i64 %839, 9
  %900 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %899
  %901 = load float, float* %900, align 4, !tbaa !133
  %902 = insertelement <8 x float> undef, float %901, i32 0
  %903 = shufflevector <8 x float> %902, <8 x float> undef, <8 x i32> zeroinitializer
  %904 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %903, <8 x float> %849, <8 x float> %828)
  %905 = add nuw nsw i64 %839, 10
  %906 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %905
  %907 = load float, float* %906, align 4, !tbaa !133
  %908 = insertelement <8 x float> undef, float %907, i32 0
  %909 = shufflevector <8 x float> %908, <8 x float> undef, <8 x i32> zeroinitializer
  %910 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %909, <8 x float> %849, <8 x float> %827)
  %911 = add nuw nsw i64 %839, 11
  %912 = getelementptr inbounds [100352 x float], [100352 x float]* %5, i64 0, i64 %911
  %913 = load float, float* %912, align 4, !tbaa !133
  %914 = insertelement <8 x float> undef, float %913, i32 0
  %915 = shufflevector <8 x float> %914, <8 x float> undef, <8 x i32> zeroinitializer
  %916 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %915, <8 x float> %849, <8 x float> %826)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 8
  br i1 %exitcond.2, label %for_end33.2, label %for_body32.2, !prof !28

for_end33.2:                                      ; preds = %for_body32.2
  %indvars.iv.next115 = add nuw nsw i64 %indvars.iv114, 1
  %917 = add nuw nsw i32 %606, 1
  %exitcond117 = icmp eq i64 %indvars.iv.next115, 3
  br i1 %exitcond117, label %for_end27, label %for_begin28.preheader, !prof !28
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_bias_add_3(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_bias_add_3_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_bias_add_3_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_begin1.preheader, %entry
  %indvars.iv1 = phi i64 [ 0, %entry ], [ %indvars.iv.next2, %for_begin1.preheader ]
  %6 = mul nuw nsw i64 %indvars.iv1, 144
  %7 = getelementptr inbounds float, float* %3, i64 %indvars.iv1
  %8 = load float, float* %7, align 4, !tbaa !160
  %9 = insertelement <4 x float> undef, float %8, i32 0
  %10 = shufflevector <4 x float> %9, <4 x float> undef, <4 x i32> zeroinitializer
  %11 = insertelement <4 x float> undef, float %8, i32 0
  %12 = shufflevector <4 x float> %11, <4 x float> undef, <4 x i32> zeroinitializer
  %13 = insertelement <4 x float> undef, float %8, i32 0
  %14 = shufflevector <4 x float> %13, <4 x float> undef, <4 x i32> zeroinitializer
  %15 = getelementptr inbounds float, float* %4, i64 %6
  %16 = getelementptr inbounds float, float* %5, i64 %6
  %17 = bitcast float* %15 to <4 x float>*
  %18 = load <4 x float>, <4 x float>* %17, align 4, !tbaa !163
  %19 = fadd <4 x float> %10, %18
  %20 = bitcast float* %16 to <4 x float>*
  store <4 x float> %19, <4 x float>* %20, align 4, !tbaa !166
  %21 = or i64 %6, 4
  %22 = getelementptr inbounds float, float* %4, i64 %21
  %23 = getelementptr inbounds float, float* %5, i64 %21
  %24 = bitcast float* %22 to <4 x float>*
  %25 = load <4 x float>, <4 x float>* %24, align 4, !tbaa !163
  %26 = fadd <4 x float> %12, %25
  %27 = bitcast float* %23 to <4 x float>*
  store <4 x float> %26, <4 x float>* %27, align 4, !tbaa !166
  %28 = or i64 %6, 8
  %29 = getelementptr inbounds float, float* %4, i64 %28
  %30 = getelementptr inbounds float, float* %5, i64 %28
  %31 = bitcast float* %29 to <4 x float>*
  %32 = load <4 x float>, <4 x float>* %31, align 4, !tbaa !163
  %33 = fadd <4 x float> %14, %32
  %34 = bitcast float* %30 to <4 x float>*
  store <4 x float> %33, <4 x float>* %34, align 4, !tbaa !166
  %35 = or i64 %6, 12
  %36 = getelementptr inbounds float, float* %4, i64 %35
  %37 = getelementptr inbounds float, float* %5, i64 %35
  %38 = bitcast float* %36 to <4 x float>*
  %39 = load <4 x float>, <4 x float>* %38, align 4, !tbaa !163
  %40 = fadd <4 x float> %10, %39
  %41 = bitcast float* %37 to <4 x float>*
  store <4 x float> %40, <4 x float>* %41, align 4, !tbaa !166
  %42 = add nuw nsw i64 %35, 4
  %43 = getelementptr inbounds float, float* %4, i64 %42
  %44 = getelementptr inbounds float, float* %5, i64 %42
  %45 = bitcast float* %43 to <4 x float>*
  %46 = load <4 x float>, <4 x float>* %45, align 4, !tbaa !163
  %47 = fadd <4 x float> %12, %46
  %48 = bitcast float* %44 to <4 x float>*
  store <4 x float> %47, <4 x float>* %48, align 4, !tbaa !166
  %49 = add nuw nsw i64 %35, 8
  %50 = getelementptr inbounds float, float* %4, i64 %49
  %51 = getelementptr inbounds float, float* %5, i64 %49
  %52 = bitcast float* %50 to <4 x float>*
  %53 = load <4 x float>, <4 x float>* %52, align 4, !tbaa !163
  %54 = fadd <4 x float> %14, %53
  %55 = bitcast float* %51 to <4 x float>*
  store <4 x float> %54, <4 x float>* %55, align 4, !tbaa !166
  %56 = add nuw nsw i64 %6, 24
  %57 = getelementptr inbounds float, float* %4, i64 %56
  %58 = getelementptr inbounds float, float* %5, i64 %56
  %59 = bitcast float* %57 to <4 x float>*
  %60 = load <4 x float>, <4 x float>* %59, align 4, !tbaa !163
  %61 = fadd <4 x float> %10, %60
  %62 = bitcast float* %58 to <4 x float>*
  store <4 x float> %61, <4 x float>* %62, align 4, !tbaa !166
  %63 = add nuw nsw i64 %6, 28
  %64 = getelementptr inbounds float, float* %4, i64 %63
  %65 = getelementptr inbounds float, float* %5, i64 %63
  %66 = bitcast float* %64 to <4 x float>*
  %67 = load <4 x float>, <4 x float>* %66, align 4, !tbaa !163
  %68 = fadd <4 x float> %12, %67
  %69 = bitcast float* %65 to <4 x float>*
  store <4 x float> %68, <4 x float>* %69, align 4, !tbaa !166
  %70 = add nuw nsw i64 %6, 32
  %71 = getelementptr inbounds float, float* %4, i64 %70
  %72 = getelementptr inbounds float, float* %5, i64 %70
  %73 = bitcast float* %71 to <4 x float>*
  %74 = load <4 x float>, <4 x float>* %73, align 4, !tbaa !163
  %75 = fadd <4 x float> %14, %74
  %76 = bitcast float* %72 to <4 x float>*
  store <4 x float> %75, <4 x float>* %76, align 4, !tbaa !166
  %77 = add nuw nsw i64 %6, 36
  %78 = getelementptr inbounds float, float* %4, i64 %77
  %79 = getelementptr inbounds float, float* %5, i64 %77
  %80 = bitcast float* %78 to <4 x float>*
  %81 = load <4 x float>, <4 x float>* %80, align 4, !tbaa !163
  %82 = fadd <4 x float> %10, %81
  %83 = bitcast float* %79 to <4 x float>*
  store <4 x float> %82, <4 x float>* %83, align 4, !tbaa !166
  %84 = add nuw nsw i64 %6, 40
  %85 = getelementptr inbounds float, float* %4, i64 %84
  %86 = getelementptr inbounds float, float* %5, i64 %84
  %87 = bitcast float* %85 to <4 x float>*
  %88 = load <4 x float>, <4 x float>* %87, align 4, !tbaa !163
  %89 = fadd <4 x float> %12, %88
  %90 = bitcast float* %86 to <4 x float>*
  store <4 x float> %89, <4 x float>* %90, align 4, !tbaa !166
  %91 = add nuw nsw i64 %6, 44
  %92 = getelementptr inbounds float, float* %4, i64 %91
  %93 = getelementptr inbounds float, float* %5, i64 %91
  %94 = bitcast float* %92 to <4 x float>*
  %95 = load <4 x float>, <4 x float>* %94, align 4, !tbaa !163
  %96 = fadd <4 x float> %14, %95
  %97 = bitcast float* %93 to <4 x float>*
  store <4 x float> %96, <4 x float>* %97, align 4, !tbaa !166
  %98 = add nuw nsw i64 %6, 48
  %99 = getelementptr inbounds float, float* %4, i64 %98
  %100 = getelementptr inbounds float, float* %5, i64 %98
  %101 = bitcast float* %99 to <4 x float>*
  %102 = load <4 x float>, <4 x float>* %101, align 4, !tbaa !163
  %103 = fadd <4 x float> %10, %102
  %104 = bitcast float* %100 to <4 x float>*
  store <4 x float> %103, <4 x float>* %104, align 4, !tbaa !166
  %105 = add nuw nsw i64 %6, 52
  %106 = getelementptr inbounds float, float* %4, i64 %105
  %107 = getelementptr inbounds float, float* %5, i64 %105
  %108 = bitcast float* %106 to <4 x float>*
  %109 = load <4 x float>, <4 x float>* %108, align 4, !tbaa !163
  %110 = fadd <4 x float> %12, %109
  %111 = bitcast float* %107 to <4 x float>*
  store <4 x float> %110, <4 x float>* %111, align 4, !tbaa !166
  %112 = add nuw nsw i64 %6, 56
  %113 = getelementptr inbounds float, float* %4, i64 %112
  %114 = getelementptr inbounds float, float* %5, i64 %112
  %115 = bitcast float* %113 to <4 x float>*
  %116 = load <4 x float>, <4 x float>* %115, align 4, !tbaa !163
  %117 = fadd <4 x float> %14, %116
  %118 = bitcast float* %114 to <4 x float>*
  store <4 x float> %117, <4 x float>* %118, align 4, !tbaa !166
  %119 = add nuw nsw i64 %6, 60
  %120 = getelementptr inbounds float, float* %4, i64 %119
  %121 = getelementptr inbounds float, float* %5, i64 %119
  %122 = bitcast float* %120 to <4 x float>*
  %123 = load <4 x float>, <4 x float>* %122, align 4, !tbaa !163
  %124 = fadd <4 x float> %10, %123
  %125 = bitcast float* %121 to <4 x float>*
  store <4 x float> %124, <4 x float>* %125, align 4, !tbaa !166
  %126 = add nuw nsw i64 %6, 64
  %127 = getelementptr inbounds float, float* %4, i64 %126
  %128 = getelementptr inbounds float, float* %5, i64 %126
  %129 = bitcast float* %127 to <4 x float>*
  %130 = load <4 x float>, <4 x float>* %129, align 4, !tbaa !163
  %131 = fadd <4 x float> %12, %130
  %132 = bitcast float* %128 to <4 x float>*
  store <4 x float> %131, <4 x float>* %132, align 4, !tbaa !166
  %133 = add nuw nsw i64 %6, 68
  %134 = getelementptr inbounds float, float* %4, i64 %133
  %135 = getelementptr inbounds float, float* %5, i64 %133
  %136 = bitcast float* %134 to <4 x float>*
  %137 = load <4 x float>, <4 x float>* %136, align 4, !tbaa !163
  %138 = fadd <4 x float> %14, %137
  %139 = bitcast float* %135 to <4 x float>*
  store <4 x float> %138, <4 x float>* %139, align 4, !tbaa !166
  %140 = add nuw nsw i64 %6, 72
  %141 = getelementptr inbounds float, float* %4, i64 %140
  %142 = getelementptr inbounds float, float* %5, i64 %140
  %143 = bitcast float* %141 to <4 x float>*
  %144 = load <4 x float>, <4 x float>* %143, align 4, !tbaa !163
  %145 = fadd <4 x float> %10, %144
  %146 = bitcast float* %142 to <4 x float>*
  store <4 x float> %145, <4 x float>* %146, align 4, !tbaa !166
  %147 = add nuw nsw i64 %6, 76
  %148 = getelementptr inbounds float, float* %4, i64 %147
  %149 = getelementptr inbounds float, float* %5, i64 %147
  %150 = bitcast float* %148 to <4 x float>*
  %151 = load <4 x float>, <4 x float>* %150, align 4, !tbaa !163
  %152 = fadd <4 x float> %12, %151
  %153 = bitcast float* %149 to <4 x float>*
  store <4 x float> %152, <4 x float>* %153, align 4, !tbaa !166
  %154 = add nuw nsw i64 %6, 80
  %155 = getelementptr inbounds float, float* %4, i64 %154
  %156 = getelementptr inbounds float, float* %5, i64 %154
  %157 = bitcast float* %155 to <4 x float>*
  %158 = load <4 x float>, <4 x float>* %157, align 4, !tbaa !163
  %159 = fadd <4 x float> %14, %158
  %160 = bitcast float* %156 to <4 x float>*
  store <4 x float> %159, <4 x float>* %160, align 4, !tbaa !166
  %161 = add nuw nsw i64 %6, 84
  %162 = getelementptr inbounds float, float* %4, i64 %161
  %163 = getelementptr inbounds float, float* %5, i64 %161
  %164 = bitcast float* %162 to <4 x float>*
  %165 = load <4 x float>, <4 x float>* %164, align 4, !tbaa !163
  %166 = fadd <4 x float> %10, %165
  %167 = bitcast float* %163 to <4 x float>*
  store <4 x float> %166, <4 x float>* %167, align 4, !tbaa !166
  %168 = add nuw nsw i64 %6, 88
  %169 = getelementptr inbounds float, float* %4, i64 %168
  %170 = getelementptr inbounds float, float* %5, i64 %168
  %171 = bitcast float* %169 to <4 x float>*
  %172 = load <4 x float>, <4 x float>* %171, align 4, !tbaa !163
  %173 = fadd <4 x float> %12, %172
  %174 = bitcast float* %170 to <4 x float>*
  store <4 x float> %173, <4 x float>* %174, align 4, !tbaa !166
  %175 = add nuw nsw i64 %6, 92
  %176 = getelementptr inbounds float, float* %4, i64 %175
  %177 = getelementptr inbounds float, float* %5, i64 %175
  %178 = bitcast float* %176 to <4 x float>*
  %179 = load <4 x float>, <4 x float>* %178, align 4, !tbaa !163
  %180 = fadd <4 x float> %14, %179
  %181 = bitcast float* %177 to <4 x float>*
  store <4 x float> %180, <4 x float>* %181, align 4, !tbaa !166
  %182 = add nuw nsw i64 %6, 96
  %183 = getelementptr inbounds float, float* %4, i64 %182
  %184 = getelementptr inbounds float, float* %5, i64 %182
  %185 = bitcast float* %183 to <4 x float>*
  %186 = load <4 x float>, <4 x float>* %185, align 4, !tbaa !163
  %187 = fadd <4 x float> %10, %186
  %188 = bitcast float* %184 to <4 x float>*
  store <4 x float> %187, <4 x float>* %188, align 4, !tbaa !166
  %189 = add nuw nsw i64 %6, 100
  %190 = getelementptr inbounds float, float* %4, i64 %189
  %191 = getelementptr inbounds float, float* %5, i64 %189
  %192 = bitcast float* %190 to <4 x float>*
  %193 = load <4 x float>, <4 x float>* %192, align 4, !tbaa !163
  %194 = fadd <4 x float> %12, %193
  %195 = bitcast float* %191 to <4 x float>*
  store <4 x float> %194, <4 x float>* %195, align 4, !tbaa !166
  %196 = add nuw nsw i64 %6, 104
  %197 = getelementptr inbounds float, float* %4, i64 %196
  %198 = getelementptr inbounds float, float* %5, i64 %196
  %199 = bitcast float* %197 to <4 x float>*
  %200 = load <4 x float>, <4 x float>* %199, align 4, !tbaa !163
  %201 = fadd <4 x float> %14, %200
  %202 = bitcast float* %198 to <4 x float>*
  store <4 x float> %201, <4 x float>* %202, align 4, !tbaa !166
  %203 = add nuw nsw i64 %6, 108
  %204 = getelementptr inbounds float, float* %4, i64 %203
  %205 = getelementptr inbounds float, float* %5, i64 %203
  %206 = bitcast float* %204 to <4 x float>*
  %207 = load <4 x float>, <4 x float>* %206, align 4, !tbaa !163
  %208 = fadd <4 x float> %10, %207
  %209 = bitcast float* %205 to <4 x float>*
  store <4 x float> %208, <4 x float>* %209, align 4, !tbaa !166
  %210 = add nuw nsw i64 %6, 112
  %211 = getelementptr inbounds float, float* %4, i64 %210
  %212 = getelementptr inbounds float, float* %5, i64 %210
  %213 = bitcast float* %211 to <4 x float>*
  %214 = load <4 x float>, <4 x float>* %213, align 4, !tbaa !163
  %215 = fadd <4 x float> %12, %214
  %216 = bitcast float* %212 to <4 x float>*
  store <4 x float> %215, <4 x float>* %216, align 4, !tbaa !166
  %217 = add nuw nsw i64 %6, 116
  %218 = getelementptr inbounds float, float* %4, i64 %217
  %219 = getelementptr inbounds float, float* %5, i64 %217
  %220 = bitcast float* %218 to <4 x float>*
  %221 = load <4 x float>, <4 x float>* %220, align 4, !tbaa !163
  %222 = fadd <4 x float> %14, %221
  %223 = bitcast float* %219 to <4 x float>*
  store <4 x float> %222, <4 x float>* %223, align 4, !tbaa !166
  %224 = add nuw nsw i64 %6, 120
  %225 = getelementptr inbounds float, float* %4, i64 %224
  %226 = getelementptr inbounds float, float* %5, i64 %224
  %227 = bitcast float* %225 to <4 x float>*
  %228 = load <4 x float>, <4 x float>* %227, align 4, !tbaa !163
  %229 = fadd <4 x float> %10, %228
  %230 = bitcast float* %226 to <4 x float>*
  store <4 x float> %229, <4 x float>* %230, align 4, !tbaa !166
  %231 = add nuw nsw i64 %6, 124
  %232 = getelementptr inbounds float, float* %4, i64 %231
  %233 = getelementptr inbounds float, float* %5, i64 %231
  %234 = bitcast float* %232 to <4 x float>*
  %235 = load <4 x float>, <4 x float>* %234, align 4, !tbaa !163
  %236 = fadd <4 x float> %12, %235
  %237 = bitcast float* %233 to <4 x float>*
  store <4 x float> %236, <4 x float>* %237, align 4, !tbaa !166
  %238 = add nuw nsw i64 %6, 128
  %239 = getelementptr inbounds float, float* %4, i64 %238
  %240 = getelementptr inbounds float, float* %5, i64 %238
  %241 = bitcast float* %239 to <4 x float>*
  %242 = load <4 x float>, <4 x float>* %241, align 4, !tbaa !163
  %243 = fadd <4 x float> %14, %242
  %244 = bitcast float* %240 to <4 x float>*
  store <4 x float> %243, <4 x float>* %244, align 4, !tbaa !166
  %245 = add nuw nsw i64 %6, 132
  %246 = getelementptr inbounds float, float* %4, i64 %245
  %247 = getelementptr inbounds float, float* %5, i64 %245
  %248 = bitcast float* %246 to <4 x float>*
  %249 = load <4 x float>, <4 x float>* %248, align 4, !tbaa !163
  %250 = fadd <4 x float> %10, %249
  %251 = bitcast float* %247 to <4 x float>*
  store <4 x float> %250, <4 x float>* %251, align 4, !tbaa !166
  %252 = add nuw nsw i64 %6, 136
  %253 = getelementptr inbounds float, float* %4, i64 %252
  %254 = getelementptr inbounds float, float* %5, i64 %252
  %255 = bitcast float* %253 to <4 x float>*
  %256 = load <4 x float>, <4 x float>* %255, align 4, !tbaa !163
  %257 = fadd <4 x float> %12, %256
  %258 = bitcast float* %254 to <4 x float>*
  store <4 x float> %257, <4 x float>* %258, align 4, !tbaa !166
  %259 = add nuw nsw i64 %6, 140
  %260 = getelementptr inbounds float, float* %4, i64 %259
  %261 = getelementptr inbounds float, float* %5, i64 %259
  %262 = bitcast float* %260 to <4 x float>*
  %263 = load <4 x float>, <4 x float>* %262, align 4, !tbaa !163
  %264 = fadd <4 x float> %14, %263
  %265 = bitcast float* %261 to <4 x float>*
  store <4 x float> %264, <4 x float>* %265, align 4, !tbaa !166
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 512
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !28

for_end:                                          ; preds = %for_begin1.preheader
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
  %4 = mul nuw nsw i64 %indvars.iv4, 36
  %5 = mul nuw nsw i64 %indvars.iv4, 144
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_begin4.preheader ]
  %6 = mul nuw nsw i64 %indvars.iv, 6
  %7 = add nuw nsw i64 %6, %4
  %8 = mul nuw nsw i64 %indvars.iv, 24
  %9 = add nuw nsw i64 %8, %5
  %10 = getelementptr inbounds float, float* %2, i64 %7
  %11 = getelementptr inbounds float, float* %3, i64 %9
  %12 = load float, float* %11, align 4, !tbaa !169
  %13 = fcmp olt float %12, 0xC7EFFFFFE0000000
  %14 = select i1 %13, float 0xC7EFFFFFE0000000, float %12
  %15 = or i64 %9, 1
  %16 = getelementptr inbounds float, float* %3, i64 %15
  %17 = load float, float* %16, align 4, !tbaa !169
  %18 = fcmp ogt float %14, %17
  %19 = select i1 %18, float %14, float %17
  %20 = add nuw nsw i64 %9, 12
  %21 = getelementptr inbounds float, float* %3, i64 %20
  %22 = load float, float* %21, align 4, !tbaa !169
  %23 = fcmp ogt float %19, %22
  %24 = select i1 %23, float %19, float %22
  %25 = add nuw nsw i64 %9, 13
  %26 = getelementptr inbounds float, float* %3, i64 %25
  %27 = load float, float* %26, align 4, !tbaa !169
  %28 = fcmp ogt float %24, %27
  %29 = select i1 %28, float %24, float %27
  store float %29, float* %10, align 4, !tbaa !172
  %30 = or i64 %7, 1
  %31 = getelementptr inbounds float, float* %2, i64 %30
  %32 = or i64 %9, 2
  %33 = getelementptr inbounds float, float* %3, i64 %32
  %34 = load float, float* %33, align 4, !tbaa !169
  %35 = fcmp olt float %34, 0xC7EFFFFFE0000000
  %36 = select i1 %35, float 0xC7EFFFFFE0000000, float %34
  %37 = or i64 %9, 3
  %38 = getelementptr inbounds float, float* %3, i64 %37
  %39 = load float, float* %38, align 4, !tbaa !169
  %40 = fcmp ogt float %36, %39
  %41 = select i1 %40, float %36, float %39
  %42 = add nuw nsw i64 %32, 12
  %43 = getelementptr inbounds float, float* %3, i64 %42
  %44 = load float, float* %43, align 4, !tbaa !169
  %45 = fcmp ogt float %41, %44
  %46 = select i1 %45, float %41, float %44
  %47 = add nuw nsw i64 %32, 13
  %48 = getelementptr inbounds float, float* %3, i64 %47
  %49 = load float, float* %48, align 4, !tbaa !169
  %50 = fcmp ogt float %46, %49
  %51 = select i1 %50, float %46, float %49
  store float %51, float* %31, align 4, !tbaa !172
  %52 = add nuw nsw i64 %7, 2
  %53 = getelementptr inbounds float, float* %2, i64 %52
  %54 = or i64 %9, 4
  %55 = getelementptr inbounds float, float* %3, i64 %54
  %56 = load float, float* %55, align 4, !tbaa !169
  %57 = fcmp olt float %56, 0xC7EFFFFFE0000000
  %58 = select i1 %57, float 0xC7EFFFFFE0000000, float %56
  %59 = or i64 %9, 5
  %60 = getelementptr inbounds float, float* %3, i64 %59
  %61 = load float, float* %60, align 4, !tbaa !169
  %62 = fcmp ogt float %58, %61
  %63 = select i1 %62, float %58, float %61
  %64 = add nuw nsw i64 %54, 12
  %65 = getelementptr inbounds float, float* %3, i64 %64
  %66 = load float, float* %65, align 4, !tbaa !169
  %67 = fcmp ogt float %63, %66
  %68 = select i1 %67, float %63, float %66
  %69 = add nuw nsw i64 %54, 13
  %70 = getelementptr inbounds float, float* %3, i64 %69
  %71 = load float, float* %70, align 4, !tbaa !169
  %72 = fcmp ogt float %68, %71
  %73 = select i1 %72, float %68, float %71
  store float %73, float* %53, align 4, !tbaa !172
  %74 = add nuw nsw i64 %7, 3
  %75 = getelementptr inbounds float, float* %2, i64 %74
  %76 = or i64 %9, 6
  %77 = getelementptr inbounds float, float* %3, i64 %76
  %78 = load float, float* %77, align 4, !tbaa !169
  %79 = fcmp olt float %78, 0xC7EFFFFFE0000000
  %80 = select i1 %79, float 0xC7EFFFFFE0000000, float %78
  %81 = or i64 %9, 7
  %82 = getelementptr inbounds float, float* %3, i64 %81
  %83 = load float, float* %82, align 4, !tbaa !169
  %84 = fcmp ogt float %80, %83
  %85 = select i1 %84, float %80, float %83
  %86 = add nuw nsw i64 %76, 12
  %87 = getelementptr inbounds float, float* %3, i64 %86
  %88 = load float, float* %87, align 4, !tbaa !169
  %89 = fcmp ogt float %85, %88
  %90 = select i1 %89, float %85, float %88
  %91 = add nuw nsw i64 %76, 13
  %92 = getelementptr inbounds float, float* %3, i64 %91
  %93 = load float, float* %92, align 4, !tbaa !169
  %94 = fcmp ogt float %90, %93
  %95 = select i1 %94, float %90, float %93
  store float %95, float* %75, align 4, !tbaa !172
  %96 = add nuw nsw i64 %7, 4
  %97 = getelementptr inbounds float, float* %2, i64 %96
  %98 = add nuw nsw i64 %9, 8
  %99 = getelementptr inbounds float, float* %3, i64 %98
  %100 = load float, float* %99, align 4, !tbaa !169
  %101 = fcmp olt float %100, 0xC7EFFFFFE0000000
  %102 = select i1 %101, float 0xC7EFFFFFE0000000, float %100
  %103 = add nuw nsw i64 %9, 9
  %104 = getelementptr inbounds float, float* %3, i64 %103
  %105 = load float, float* %104, align 4, !tbaa !169
  %106 = fcmp ogt float %102, %105
  %107 = select i1 %106, float %102, float %105
  %108 = add nuw nsw i64 %9, 20
  %109 = getelementptr inbounds float, float* %3, i64 %108
  %110 = load float, float* %109, align 4, !tbaa !169
  %111 = fcmp ogt float %107, %110
  %112 = select i1 %111, float %107, float %110
  %113 = add nuw nsw i64 %9, 21
  %114 = getelementptr inbounds float, float* %3, i64 %113
  %115 = load float, float* %114, align 4, !tbaa !169
  %116 = fcmp ogt float %112, %115
  %117 = select i1 %116, float %112, float %115
  store float %117, float* %97, align 4, !tbaa !172
  %118 = add nuw nsw i64 %7, 5
  %119 = getelementptr inbounds float, float* %2, i64 %118
  %120 = add nuw nsw i64 %9, 10
  %121 = getelementptr inbounds float, float* %3, i64 %120
  %122 = load float, float* %121, align 4, !tbaa !169
  %123 = fcmp olt float %122, 0xC7EFFFFFE0000000
  %124 = select i1 %123, float 0xC7EFFFFFE0000000, float %122
  %125 = add nuw nsw i64 %9, 11
  %126 = getelementptr inbounds float, float* %3, i64 %125
  %127 = load float, float* %126, align 4, !tbaa !169
  %128 = fcmp ogt float %124, %127
  %129 = select i1 %128, float %124, float %127
  %130 = add nuw nsw i64 %9, 22
  %131 = getelementptr inbounds float, float* %3, i64 %130
  %132 = load float, float* %131, align 4, !tbaa !169
  %133 = fcmp ogt float %129, %132
  %134 = select i1 %133, float %129, float %132
  %135 = add nuw nsw i64 %9, 23
  %136 = getelementptr inbounds float, float* %3, i64 %135
  %137 = load float, float* %136, align 4, !tbaa !169
  %138 = fcmp ogt float %134, %137
  %139 = select i1 %138, float %134, float %137
  store float %139, float* %119, align 4, !tbaa !172
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 6
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 512
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !28
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
  %wide.load = load <4 x float>, <4 x float>* %5, align 4, !tbaa !175
  %6 = getelementptr inbounds float, float* %4, i64 4
  %7 = bitcast float* %6 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %7, align 4, !tbaa !175
  %8 = fcmp ogt <4 x float> %wide.load, zeroinitializer
  %9 = fcmp ogt <4 x float> %wide.load2, zeroinitializer
  %10 = select <4 x i1> %8, <4 x float> %wide.load, <4 x float> zeroinitializer
  %11 = select <4 x i1> %9, <4 x float> %wide.load2, <4 x float> zeroinitializer
  %12 = getelementptr inbounds float, float* %3, i64 %index
  %13 = bitcast float* %12 to <4 x float>*
  store <4 x float> %10, <4 x float>* %13, align 4, !tbaa !178
  %14 = getelementptr inbounds float, float* %12, i64 4
  %15 = bitcast float* %14 to <4 x float>*
  store <4 x float> %11, <4 x float>* %15, align 4, !tbaa !178
  %index.next = add i64 %index, 8
  %16 = icmp eq i64 %index.next, 1024
  br i1 %16, label %for_end, label %vector.body, !llvm.loop !181

for_end:                                          ; preds = %vector.body
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_multiply_4(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_multiply_4_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_multiply_4_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = load float, float* %4, align 64, !tbaa !182
  %6 = bitcast i8* %0 to float*
  %broadcast.splatinsert3 = insertelement <4 x float> undef, float %5, i32 0
  %broadcast.splat4 = shufflevector <4 x float> %broadcast.splatinsert3, <4 x float> undef, <4 x i32> zeroinitializer
  %broadcast.splatinsert5 = insertelement <4 x float> undef, float %5, i32 0
  %broadcast.splat6 = shufflevector <4 x float> %broadcast.splatinsert5, <4 x float> undef, <4 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ]
  %7 = getelementptr inbounds float, float* %3, i64 %index
  %8 = bitcast float* %7 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !196
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !196
  %11 = fmul <4 x float> %wide.load, %broadcast.splat4
  %12 = fmul <4 x float> %wide.load2, %broadcast.splat6
  %13 = getelementptr inbounds float, float* %6, i64 %index
  %14 = bitcast float* %13 to <4 x float>*
  store <4 x float> %11, <4 x float>* %14, align 4, !tbaa !199
  %15 = getelementptr inbounds float, float* %13, i64 4
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %12, <4 x float>* %16, align 4, !tbaa !199
  %index.next = add i64 %index, 8
  %17 = icmp eq i64 %index.next, 1024
  br i1 %17, label %for_end, label %vector.body, !llvm.loop !202

for_end:                                          ; preds = %vector.body
  ret void
}

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
  %3 = alloca [25 x <8 x float>], align 32
  %4 = alloca [76800 x <8 x float>], align 16
  %5 = alloca [269664 x float], align 16
  %.sub = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_begin1.preheader, %entry
  %indvar = phi i64 [ 0, %entry ], [ %indvar.next, %for_begin1.preheader ]
  %6 = trunc i64 %indvar to i32
  %7 = mul nuw nsw i64 %indvar, 424
  %8 = trunc i64 %indvar to i32
  %9 = mul i32 %8, 54
  %10 = udiv i32 %6, 53
  %11 = mul i32 %10, 20466
  %12 = add i32 %9, %11
  %13 = zext i32 %12 to i64
  %14 = shl nuw nsw i64 %13, 2
  %scevgep = getelementptr [269664 x float], [269664 x float]* %5, i64 0, i64 %7
  %scevgep248 = bitcast float* %scevgep to i8*
  %scevgep249 = getelementptr i8, i8* %0, i64 %14
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep248, i8* align 4 %scevgep249, i64 212, i1 false)
  %15 = add nuw nsw i64 %7, 53
  %scevgep.1 = getelementptr [269664 x float], [269664 x float]* %5, i64 0, i64 %15
  %scevgep248.1 = bitcast float* %scevgep.1 to i8*
  %16 = add nuw nsw i64 %14, 11664
  %scevgep249.1 = getelementptr i8, i8* %0, i64 %16
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep248.1, i8* align 4 %scevgep249.1, i64 212, i1 false)
  %17 = add nuw nsw i64 %7, 106
  %scevgep.2 = getelementptr [269664 x float], [269664 x float]* %5, i64 0, i64 %17
  %scevgep248.2 = bitcast float* %scevgep.2 to i8*
  %18 = add nuw nsw i64 %14, 23328
  %scevgep249.2 = getelementptr i8, i8* %0, i64 %18
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep248.2, i8* align 4 %scevgep249.2, i64 212, i1 false)
  %19 = add nuw nsw i64 %7, 159
  %scevgep.3 = getelementptr [269664 x float], [269664 x float]* %5, i64 0, i64 %19
  %scevgep248.3 = bitcast float* %scevgep.3 to i8*
  %20 = add nuw nsw i64 %14, 34992
  %scevgep249.3 = getelementptr i8, i8* %0, i64 %20
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep248.3, i8* align 4 %scevgep249.3, i64 212, i1 false)
  %21 = add nuw nsw i64 %7, 212
  %scevgep.4 = getelementptr [269664 x float], [269664 x float]* %5, i64 0, i64 %21
  %scevgep248.4 = bitcast float* %scevgep.4 to i8*
  %22 = add nuw nsw i64 %14, 46656
  %scevgep249.4 = getelementptr i8, i8* %0, i64 %22
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep248.4, i8* align 4 %scevgep249.4, i64 212, i1 false)
  %23 = add nuw nsw i64 %7, 265
  %scevgep.5 = getelementptr [269664 x float], [269664 x float]* %5, i64 0, i64 %23
  %scevgep248.5 = bitcast float* %scevgep.5 to i8*
  %24 = add nuw nsw i64 %14, 58320
  %scevgep249.5 = getelementptr i8, i8* %0, i64 %24
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep248.5, i8* align 4 %scevgep249.5, i64 212, i1 false)
  %25 = add nuw nsw i64 %7, 318
  %scevgep.6 = getelementptr [269664 x float], [269664 x float]* %5, i64 0, i64 %25
  %scevgep248.6 = bitcast float* %scevgep.6 to i8*
  %26 = add nuw nsw i64 %14, 69984
  %scevgep249.6 = getelementptr i8, i8* %0, i64 %26
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep248.6, i8* align 4 %scevgep249.6, i64 212, i1 false)
  %27 = add nuw nsw i64 %7, 371
  %scevgep.7 = getelementptr [269664 x float], [269664 x float]* %5, i64 0, i64 %27
  %scevgep248.7 = bitcast float* %scevgep.7 to i8*
  %28 = add nuw nsw i64 %14, 81648
  %scevgep249.7 = getelementptr i8, i8* %0, i64 %28
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep248.7, i8* align 4 %scevgep249.7, i64 212, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond251 = icmp eq i64 %indvar.next, 636
  br i1 %exitcond251, label %for_begin7.preheader, label %for_begin1.preheader, !prof !28

for_begin7.preheader:                             ; preds = %for_begin1.preheader
  %29 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %30 = phi i32 [ 0, %for_begin7.preheader ], [ %94, %for_end12 ]
  %31 = urem i32 %30, 5
  %32 = mul nuw nsw i32 %31, 320
  %33 = udiv i32 %30, 5
  %34 = mul nsw i32 %33, 19200
  %35 = add nuw i32 %32, %34
  %36 = mul nuw nsw i32 %31, 5
  %37 = or i32 %36, %34
  %38 = zext i32 %37 to i64
  %39 = zext i32 %35 to i64
  br label %for_begin13.preheader

for_begin19.preheader:                            ; preds = %for_end12
  %40 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 8
  %41 = bitcast float* %40 to <8 x float>*
  %42 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 16
  %43 = bitcast float* %42 to <8 x float>*
  %44 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 24
  %45 = bitcast float* %44 to <8 x float>*
  %46 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 32
  %47 = bitcast float* %46 to <8 x float>*
  %48 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 40
  %49 = bitcast float* %48 to <8 x float>*
  %50 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 48
  %51 = bitcast float* %50 to <8 x float>*
  %52 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 56
  %53 = bitcast float* %52 to <8 x float>*
  %54 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 64
  %55 = bitcast float* %54 to <8 x float>*
  %56 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 72
  %57 = bitcast float* %56 to <8 x float>*
  %58 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 80
  %59 = bitcast float* %58 to <8 x float>*
  %60 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 88
  %61 = bitcast float* %60 to <8 x float>*
  %62 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 96
  %63 = bitcast float* %62 to <8 x float>*
  %64 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 104
  %65 = bitcast float* %64 to <8 x float>*
  %66 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 112
  %67 = bitcast float* %66 to <8 x float>*
  %68 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 120
  %69 = bitcast float* %68 to <8 x float>*
  %70 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 128
  %71 = bitcast float* %70 to <8 x float>*
  %72 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 136
  %73 = bitcast float* %72 to <8 x float>*
  %74 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 144
  %75 = bitcast float* %74 to <8 x float>*
  %76 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 152
  %77 = bitcast float* %76 to <8 x float>*
  %78 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 160
  %79 = bitcast float* %78 to <8 x float>*
  %80 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 168
  %81 = bitcast float* %80 to <8 x float>*
  %82 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 176
  %83 = bitcast float* %82 to <8 x float>*
  %84 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 184
  %85 = bitcast float* %84 to <8 x float>*
  %86 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 192
  %87 = bitcast float* %86 to <8 x float>*
  %88 = bitcast i8* %2 to float*
  %89 = bitcast [25 x <8 x float>]* %3 to i8*
  br label %for_body20

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvars.iv239 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next240, %for_end15 ]
  %90 = mul nuw nsw i64 %indvars.iv239, 1600
  %91 = add nuw nsw i64 %90, %39
  %92 = mul nuw nsw i64 %indvars.iv239, 200
  %93 = add nuw nsw i64 %92, %38
  br label %for_begin16.preheader

for_end12:                                        ; preds = %for_end15
  %94 = add nuw nsw i32 %30, 1
  %exitcond242 = icmp eq i32 %94, 160
  br i1 %exitcond242, label %for_begin19.preheader, label %for_begin10.preheader, !prof !28

for_begin16.preheader:                            ; preds = %for_begin16.preheader, %for_begin13.preheader
  %indvars.iv236 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next237, %for_begin16.preheader ]
  %95 = shl i64 %indvars.iv236, 6
  %96 = add nuw nsw i64 %91, %95
  %97 = add nuw nsw i64 %93, %indvars.iv236
  %98 = getelementptr inbounds float, float* %29, i64 %97
  %99 = load float, float* %98, align 4, !tbaa !203
  %100 = insertelement <8 x float> undef, float %99, i32 0
  %101 = shl i64 %97, 32
  %sext = add i64 %101, 10307921510400
  %102 = ashr exact i64 %sext, 32
  %103 = getelementptr inbounds float, float* %29, i64 %102
  %104 = load float, float* %103, align 4, !tbaa !203
  %105 = insertelement <8 x float> %100, float %104, i32 1
  %106 = shl i64 %97, 32
  %sext252 = add i64 %106, 20615843020800
  %107 = ashr exact i64 %sext252, 32
  %108 = getelementptr inbounds float, float* %29, i64 %107
  %109 = load float, float* %108, align 4, !tbaa !203
  %110 = insertelement <8 x float> %105, float %109, i32 2
  %111 = shl i64 %97, 32
  %sext253 = add i64 %111, 30923764531200
  %112 = ashr exact i64 %sext253, 32
  %113 = getelementptr inbounds float, float* %29, i64 %112
  %114 = load float, float* %113, align 4, !tbaa !203
  %115 = insertelement <8 x float> %110, float %114, i32 3
  %116 = shl i64 %97, 32
  %sext254 = add i64 %116, 41231686041600
  %117 = ashr exact i64 %sext254, 32
  %118 = getelementptr inbounds float, float* %29, i64 %117
  %119 = load float, float* %118, align 4, !tbaa !203
  %120 = insertelement <8 x float> %115, float %119, i32 4
  %121 = shl i64 %97, 32
  %sext255 = add i64 %121, 51539607552000
  %122 = ashr exact i64 %sext255, 32
  %123 = getelementptr inbounds float, float* %29, i64 %122
  %124 = load float, float* %123, align 4, !tbaa !203
  %125 = insertelement <8 x float> %120, float %124, i32 5
  %126 = shl i64 %97, 32
  %sext256 = add i64 %126, 61847529062400
  %127 = ashr exact i64 %sext256, 32
  %128 = getelementptr inbounds float, float* %29, i64 %127
  %129 = load float, float* %128, align 4, !tbaa !203
  %130 = insertelement <8 x float> %125, float %129, i32 6
  %131 = shl i64 %97, 32
  %sext257 = add i64 %131, 72155450572800
  %132 = ashr exact i64 %sext257, 32
  %133 = getelementptr inbounds float, float* %29, i64 %132
  %134 = load float, float* %133, align 4, !tbaa !203
  %135 = insertelement <8 x float> %130, float %134, i32 7
  %136 = getelementptr inbounds [76800 x <8 x float>], [76800 x <8 x float>]* %4, i64 0, i64 0, i64 %96
  %137 = bitcast float* %136 to <8 x float>*
  store <8 x float> %135, <8 x float>* %137, align 16, !tbaa !206
  %138 = or i64 %96, 8
  %139 = add nuw nsw i64 %97, 25
  %140 = getelementptr inbounds float, float* %29, i64 %139
  %141 = load float, float* %140, align 4, !tbaa !203
  %142 = insertelement <8 x float> undef, float %141, i32 0
  %143 = shl i64 %97, 32
  %sext258 = add i64 %143, 10415295692800
  %144 = ashr exact i64 %sext258, 32
  %145 = getelementptr inbounds float, float* %29, i64 %144
  %146 = load float, float* %145, align 4, !tbaa !203
  %147 = insertelement <8 x float> %142, float %146, i32 1
  %148 = shl i64 %97, 32
  %sext259 = add i64 %148, 20723217203200
  %149 = ashr exact i64 %sext259, 32
  %150 = getelementptr inbounds float, float* %29, i64 %149
  %151 = load float, float* %150, align 4, !tbaa !203
  %152 = insertelement <8 x float> %147, float %151, i32 2
  %153 = shl i64 %97, 32
  %sext260 = add i64 %153, 31031138713600
  %154 = ashr exact i64 %sext260, 32
  %155 = getelementptr inbounds float, float* %29, i64 %154
  %156 = load float, float* %155, align 4, !tbaa !203
  %157 = insertelement <8 x float> %152, float %156, i32 3
  %158 = shl i64 %97, 32
  %sext261 = add i64 %158, 41339060224000
  %159 = ashr exact i64 %sext261, 32
  %160 = getelementptr inbounds float, float* %29, i64 %159
  %161 = load float, float* %160, align 4, !tbaa !203
  %162 = insertelement <8 x float> %157, float %161, i32 4
  %163 = shl i64 %97, 32
  %sext262 = add i64 %163, 51646981734400
  %164 = ashr exact i64 %sext262, 32
  %165 = getelementptr inbounds float, float* %29, i64 %164
  %166 = load float, float* %165, align 4, !tbaa !203
  %167 = insertelement <8 x float> %162, float %166, i32 5
  %168 = shl i64 %97, 32
  %sext263 = add i64 %168, 61954903244800
  %169 = ashr exact i64 %sext263, 32
  %170 = getelementptr inbounds float, float* %29, i64 %169
  %171 = load float, float* %170, align 4, !tbaa !203
  %172 = insertelement <8 x float> %167, float %171, i32 6
  %173 = shl i64 %97, 32
  %sext264 = add i64 %173, 72262824755200
  %174 = ashr exact i64 %sext264, 32
  %175 = getelementptr inbounds float, float* %29, i64 %174
  %176 = load float, float* %175, align 4, !tbaa !203
  %177 = insertelement <8 x float> %172, float %176, i32 7
  %178 = getelementptr inbounds [76800 x <8 x float>], [76800 x <8 x float>]* %4, i64 0, i64 0, i64 %138
  %179 = bitcast float* %178 to <8 x float>*
  store <8 x float> %177, <8 x float>* %179, align 16, !tbaa !206
  %180 = or i64 %96, 16
  %181 = add nuw nsw i64 %97, 50
  %182 = getelementptr inbounds float, float* %29, i64 %181
  %183 = load float, float* %182, align 4, !tbaa !203
  %184 = insertelement <8 x float> undef, float %183, i32 0
  %185 = shl i64 %97, 32
  %sext265 = add i64 %185, 10522669875200
  %186 = ashr exact i64 %sext265, 32
  %187 = getelementptr inbounds float, float* %29, i64 %186
  %188 = load float, float* %187, align 4, !tbaa !203
  %189 = insertelement <8 x float> %184, float %188, i32 1
  %190 = shl i64 %97, 32
  %sext266 = add i64 %190, 20830591385600
  %191 = ashr exact i64 %sext266, 32
  %192 = getelementptr inbounds float, float* %29, i64 %191
  %193 = load float, float* %192, align 4, !tbaa !203
  %194 = insertelement <8 x float> %189, float %193, i32 2
  %195 = shl i64 %97, 32
  %sext267 = add i64 %195, 31138512896000
  %196 = ashr exact i64 %sext267, 32
  %197 = getelementptr inbounds float, float* %29, i64 %196
  %198 = load float, float* %197, align 4, !tbaa !203
  %199 = insertelement <8 x float> %194, float %198, i32 3
  %200 = shl i64 %97, 32
  %sext268 = add i64 %200, 41446434406400
  %201 = ashr exact i64 %sext268, 32
  %202 = getelementptr inbounds float, float* %29, i64 %201
  %203 = load float, float* %202, align 4, !tbaa !203
  %204 = insertelement <8 x float> %199, float %203, i32 4
  %205 = shl i64 %97, 32
  %sext269 = add i64 %205, 51754355916800
  %206 = ashr exact i64 %sext269, 32
  %207 = getelementptr inbounds float, float* %29, i64 %206
  %208 = load float, float* %207, align 4, !tbaa !203
  %209 = insertelement <8 x float> %204, float %208, i32 5
  %210 = shl i64 %97, 32
  %sext270 = add i64 %210, 62062277427200
  %211 = ashr exact i64 %sext270, 32
  %212 = getelementptr inbounds float, float* %29, i64 %211
  %213 = load float, float* %212, align 4, !tbaa !203
  %214 = insertelement <8 x float> %209, float %213, i32 6
  %215 = shl i64 %97, 32
  %sext271 = add i64 %215, 72370198937600
  %216 = ashr exact i64 %sext271, 32
  %217 = getelementptr inbounds float, float* %29, i64 %216
  %218 = load float, float* %217, align 4, !tbaa !203
  %219 = insertelement <8 x float> %214, float %218, i32 7
  %220 = getelementptr inbounds [76800 x <8 x float>], [76800 x <8 x float>]* %4, i64 0, i64 0, i64 %180
  %221 = bitcast float* %220 to <8 x float>*
  store <8 x float> %219, <8 x float>* %221, align 16, !tbaa !206
  %222 = or i64 %96, 24
  %223 = add nuw nsw i64 %97, 75
  %224 = getelementptr inbounds float, float* %29, i64 %223
  %225 = load float, float* %224, align 4, !tbaa !203
  %226 = insertelement <8 x float> undef, float %225, i32 0
  %227 = shl i64 %97, 32
  %sext272 = add i64 %227, 10630044057600
  %228 = ashr exact i64 %sext272, 32
  %229 = getelementptr inbounds float, float* %29, i64 %228
  %230 = load float, float* %229, align 4, !tbaa !203
  %231 = insertelement <8 x float> %226, float %230, i32 1
  %232 = shl i64 %97, 32
  %sext273 = add i64 %232, 20937965568000
  %233 = ashr exact i64 %sext273, 32
  %234 = getelementptr inbounds float, float* %29, i64 %233
  %235 = load float, float* %234, align 4, !tbaa !203
  %236 = insertelement <8 x float> %231, float %235, i32 2
  %237 = shl i64 %97, 32
  %sext274 = add i64 %237, 31245887078400
  %238 = ashr exact i64 %sext274, 32
  %239 = getelementptr inbounds float, float* %29, i64 %238
  %240 = load float, float* %239, align 4, !tbaa !203
  %241 = insertelement <8 x float> %236, float %240, i32 3
  %242 = shl i64 %97, 32
  %sext275 = add i64 %242, 41553808588800
  %243 = ashr exact i64 %sext275, 32
  %244 = getelementptr inbounds float, float* %29, i64 %243
  %245 = load float, float* %244, align 4, !tbaa !203
  %246 = insertelement <8 x float> %241, float %245, i32 4
  %247 = shl i64 %97, 32
  %sext276 = add i64 %247, 51861730099200
  %248 = ashr exact i64 %sext276, 32
  %249 = getelementptr inbounds float, float* %29, i64 %248
  %250 = load float, float* %249, align 4, !tbaa !203
  %251 = insertelement <8 x float> %246, float %250, i32 5
  %252 = shl i64 %97, 32
  %sext277 = add i64 %252, 62169651609600
  %253 = ashr exact i64 %sext277, 32
  %254 = getelementptr inbounds float, float* %29, i64 %253
  %255 = load float, float* %254, align 4, !tbaa !203
  %256 = insertelement <8 x float> %251, float %255, i32 6
  %257 = shl i64 %97, 32
  %sext278 = add i64 %257, 72477573120000
  %258 = ashr exact i64 %sext278, 32
  %259 = getelementptr inbounds float, float* %29, i64 %258
  %260 = load float, float* %259, align 4, !tbaa !203
  %261 = insertelement <8 x float> %256, float %260, i32 7
  %262 = getelementptr inbounds [76800 x <8 x float>], [76800 x <8 x float>]* %4, i64 0, i64 0, i64 %222
  %263 = bitcast float* %262 to <8 x float>*
  store <8 x float> %261, <8 x float>* %263, align 16, !tbaa !206
  %264 = or i64 %96, 32
  %265 = add nuw nsw i64 %97, 100
  %266 = getelementptr inbounds float, float* %29, i64 %265
  %267 = load float, float* %266, align 4, !tbaa !203
  %268 = insertelement <8 x float> undef, float %267, i32 0
  %269 = shl i64 %97, 32
  %sext279 = add i64 %269, 10737418240000
  %270 = ashr exact i64 %sext279, 32
  %271 = getelementptr inbounds float, float* %29, i64 %270
  %272 = load float, float* %271, align 4, !tbaa !203
  %273 = insertelement <8 x float> %268, float %272, i32 1
  %274 = shl i64 %97, 32
  %sext280 = add i64 %274, 21045339750400
  %275 = ashr exact i64 %sext280, 32
  %276 = getelementptr inbounds float, float* %29, i64 %275
  %277 = load float, float* %276, align 4, !tbaa !203
  %278 = insertelement <8 x float> %273, float %277, i32 2
  %279 = shl i64 %97, 32
  %sext281 = add i64 %279, 31353261260800
  %280 = ashr exact i64 %sext281, 32
  %281 = getelementptr inbounds float, float* %29, i64 %280
  %282 = load float, float* %281, align 4, !tbaa !203
  %283 = insertelement <8 x float> %278, float %282, i32 3
  %284 = shl i64 %97, 32
  %sext282 = add i64 %284, 41661182771200
  %285 = ashr exact i64 %sext282, 32
  %286 = getelementptr inbounds float, float* %29, i64 %285
  %287 = load float, float* %286, align 4, !tbaa !203
  %288 = insertelement <8 x float> %283, float %287, i32 4
  %289 = shl i64 %97, 32
  %sext283 = add i64 %289, 51969104281600
  %290 = ashr exact i64 %sext283, 32
  %291 = getelementptr inbounds float, float* %29, i64 %290
  %292 = load float, float* %291, align 4, !tbaa !203
  %293 = insertelement <8 x float> %288, float %292, i32 5
  %294 = shl i64 %97, 32
  %sext284 = add i64 %294, 62277025792000
  %295 = ashr exact i64 %sext284, 32
  %296 = getelementptr inbounds float, float* %29, i64 %295
  %297 = load float, float* %296, align 4, !tbaa !203
  %298 = insertelement <8 x float> %293, float %297, i32 6
  %299 = shl i64 %97, 32
  %sext285 = add i64 %299, 72584947302400
  %300 = ashr exact i64 %sext285, 32
  %301 = getelementptr inbounds float, float* %29, i64 %300
  %302 = load float, float* %301, align 4, !tbaa !203
  %303 = insertelement <8 x float> %298, float %302, i32 7
  %304 = getelementptr inbounds [76800 x <8 x float>], [76800 x <8 x float>]* %4, i64 0, i64 0, i64 %264
  %305 = bitcast float* %304 to <8 x float>*
  store <8 x float> %303, <8 x float>* %305, align 16, !tbaa !206
  %306 = or i64 %96, 40
  %307 = add nuw nsw i64 %97, 125
  %308 = getelementptr inbounds float, float* %29, i64 %307
  %309 = load float, float* %308, align 4, !tbaa !203
  %310 = insertelement <8 x float> undef, float %309, i32 0
  %311 = shl i64 %97, 32
  %sext286 = add i64 %311, 10844792422400
  %312 = ashr exact i64 %sext286, 32
  %313 = getelementptr inbounds float, float* %29, i64 %312
  %314 = load float, float* %313, align 4, !tbaa !203
  %315 = insertelement <8 x float> %310, float %314, i32 1
  %316 = shl i64 %97, 32
  %sext287 = add i64 %316, 21152713932800
  %317 = ashr exact i64 %sext287, 32
  %318 = getelementptr inbounds float, float* %29, i64 %317
  %319 = load float, float* %318, align 4, !tbaa !203
  %320 = insertelement <8 x float> %315, float %319, i32 2
  %321 = shl i64 %97, 32
  %sext288 = add i64 %321, 31460635443200
  %322 = ashr exact i64 %sext288, 32
  %323 = getelementptr inbounds float, float* %29, i64 %322
  %324 = load float, float* %323, align 4, !tbaa !203
  %325 = insertelement <8 x float> %320, float %324, i32 3
  %326 = shl i64 %97, 32
  %sext289 = add i64 %326, 41768556953600
  %327 = ashr exact i64 %sext289, 32
  %328 = getelementptr inbounds float, float* %29, i64 %327
  %329 = load float, float* %328, align 4, !tbaa !203
  %330 = insertelement <8 x float> %325, float %329, i32 4
  %331 = shl i64 %97, 32
  %sext290 = add i64 %331, 52076478464000
  %332 = ashr exact i64 %sext290, 32
  %333 = getelementptr inbounds float, float* %29, i64 %332
  %334 = load float, float* %333, align 4, !tbaa !203
  %335 = insertelement <8 x float> %330, float %334, i32 5
  %336 = shl i64 %97, 32
  %sext291 = add i64 %336, 62384399974400
  %337 = ashr exact i64 %sext291, 32
  %338 = getelementptr inbounds float, float* %29, i64 %337
  %339 = load float, float* %338, align 4, !tbaa !203
  %340 = insertelement <8 x float> %335, float %339, i32 6
  %341 = shl i64 %97, 32
  %sext292 = add i64 %341, 72692321484800
  %342 = ashr exact i64 %sext292, 32
  %343 = getelementptr inbounds float, float* %29, i64 %342
  %344 = load float, float* %343, align 4, !tbaa !203
  %345 = insertelement <8 x float> %340, float %344, i32 7
  %346 = getelementptr inbounds [76800 x <8 x float>], [76800 x <8 x float>]* %4, i64 0, i64 0, i64 %306
  %347 = bitcast float* %346 to <8 x float>*
  store <8 x float> %345, <8 x float>* %347, align 16, !tbaa !206
  %348 = or i64 %96, 48
  %349 = add nuw nsw i64 %97, 150
  %350 = getelementptr inbounds float, float* %29, i64 %349
  %351 = load float, float* %350, align 4, !tbaa !203
  %352 = insertelement <8 x float> undef, float %351, i32 0
  %353 = shl i64 %97, 32
  %sext293 = add i64 %353, 10952166604800
  %354 = ashr exact i64 %sext293, 32
  %355 = getelementptr inbounds float, float* %29, i64 %354
  %356 = load float, float* %355, align 4, !tbaa !203
  %357 = insertelement <8 x float> %352, float %356, i32 1
  %358 = shl i64 %97, 32
  %sext294 = add i64 %358, 21260088115200
  %359 = ashr exact i64 %sext294, 32
  %360 = getelementptr inbounds float, float* %29, i64 %359
  %361 = load float, float* %360, align 4, !tbaa !203
  %362 = insertelement <8 x float> %357, float %361, i32 2
  %363 = shl i64 %97, 32
  %sext295 = add i64 %363, 31568009625600
  %364 = ashr exact i64 %sext295, 32
  %365 = getelementptr inbounds float, float* %29, i64 %364
  %366 = load float, float* %365, align 4, !tbaa !203
  %367 = insertelement <8 x float> %362, float %366, i32 3
  %368 = shl i64 %97, 32
  %sext296 = add i64 %368, 41875931136000
  %369 = ashr exact i64 %sext296, 32
  %370 = getelementptr inbounds float, float* %29, i64 %369
  %371 = load float, float* %370, align 4, !tbaa !203
  %372 = insertelement <8 x float> %367, float %371, i32 4
  %373 = shl i64 %97, 32
  %sext297 = add i64 %373, 52183852646400
  %374 = ashr exact i64 %sext297, 32
  %375 = getelementptr inbounds float, float* %29, i64 %374
  %376 = load float, float* %375, align 4, !tbaa !203
  %377 = insertelement <8 x float> %372, float %376, i32 5
  %378 = shl i64 %97, 32
  %sext298 = add i64 %378, 62491774156800
  %379 = ashr exact i64 %sext298, 32
  %380 = getelementptr inbounds float, float* %29, i64 %379
  %381 = load float, float* %380, align 4, !tbaa !203
  %382 = insertelement <8 x float> %377, float %381, i32 6
  %383 = shl i64 %97, 32
  %sext299 = add i64 %383, 72799695667200
  %384 = ashr exact i64 %sext299, 32
  %385 = getelementptr inbounds float, float* %29, i64 %384
  %386 = load float, float* %385, align 4, !tbaa !203
  %387 = insertelement <8 x float> %382, float %386, i32 7
  %388 = getelementptr inbounds [76800 x <8 x float>], [76800 x <8 x float>]* %4, i64 0, i64 0, i64 %348
  %389 = bitcast float* %388 to <8 x float>*
  store <8 x float> %387, <8 x float>* %389, align 16, !tbaa !206
  %390 = or i64 %96, 56
  %391 = add nuw nsw i64 %97, 175
  %392 = getelementptr inbounds float, float* %29, i64 %391
  %393 = load float, float* %392, align 4, !tbaa !203
  %394 = insertelement <8 x float> undef, float %393, i32 0
  %395 = shl i64 %97, 32
  %sext300 = add i64 %395, 11059540787200
  %396 = ashr exact i64 %sext300, 32
  %397 = getelementptr inbounds float, float* %29, i64 %396
  %398 = load float, float* %397, align 4, !tbaa !203
  %399 = insertelement <8 x float> %394, float %398, i32 1
  %400 = shl i64 %97, 32
  %sext301 = add i64 %400, 21367462297600
  %401 = ashr exact i64 %sext301, 32
  %402 = getelementptr inbounds float, float* %29, i64 %401
  %403 = load float, float* %402, align 4, !tbaa !203
  %404 = insertelement <8 x float> %399, float %403, i32 2
  %405 = shl i64 %97, 32
  %sext302 = add i64 %405, 31675383808000
  %406 = ashr exact i64 %sext302, 32
  %407 = getelementptr inbounds float, float* %29, i64 %406
  %408 = load float, float* %407, align 4, !tbaa !203
  %409 = insertelement <8 x float> %404, float %408, i32 3
  %410 = shl i64 %97, 32
  %sext303 = add i64 %410, 41983305318400
  %411 = ashr exact i64 %sext303, 32
  %412 = getelementptr inbounds float, float* %29, i64 %411
  %413 = load float, float* %412, align 4, !tbaa !203
  %414 = insertelement <8 x float> %409, float %413, i32 4
  %415 = shl i64 %97, 32
  %sext304 = add i64 %415, 52291226828800
  %416 = ashr exact i64 %sext304, 32
  %417 = getelementptr inbounds float, float* %29, i64 %416
  %418 = load float, float* %417, align 4, !tbaa !203
  %419 = insertelement <8 x float> %414, float %418, i32 5
  %420 = shl i64 %97, 32
  %sext305 = add i64 %420, 62599148339200
  %421 = ashr exact i64 %sext305, 32
  %422 = getelementptr inbounds float, float* %29, i64 %421
  %423 = load float, float* %422, align 4, !tbaa !203
  %424 = insertelement <8 x float> %419, float %423, i32 6
  %425 = shl i64 %97, 32
  %sext306 = add i64 %425, 72907069849600
  %426 = ashr exact i64 %sext306, 32
  %427 = getelementptr inbounds float, float* %29, i64 %426
  %428 = load float, float* %427, align 4, !tbaa !203
  %429 = insertelement <8 x float> %424, float %428, i32 7
  %430 = getelementptr inbounds [76800 x <8 x float>], [76800 x <8 x float>]* %4, i64 0, i64 0, i64 %390
  %431 = bitcast float* %430 to <8 x float>*
  store <8 x float> %429, <8 x float>* %431, align 16, !tbaa !206
  %indvars.iv.next237 = add nuw nsw i64 %indvars.iv236, 1
  %exitcond238 = icmp eq i64 %indvars.iv.next237, 5
  br i1 %exitcond238, label %for_end15, label %for_begin16.preheader, !prof !28

for_end15:                                        ; preds = %for_begin16.preheader
  %indvars.iv.next240 = add nuw nsw i64 %indvars.iv239, 1
  %exitcond241 = icmp eq i64 %indvars.iv.next240, 12
  br i1 %exitcond241, label %for_end12, label %for_begin13.preheader, !prof !28

for_body20:                                       ; preds = %for_end36, %for_begin19.preheader
  %432 = phi i32 [ 0, %for_begin19.preheader ], [ %664, %for_end36 ]
  %433 = urem i32 %432, 25
  %434 = mul nuw nsw i32 %433, 848
  %435 = udiv i32 %432, 25
  %436 = mul nsw i32 %435, 19200
  %437 = zext i32 %436 to i64
  %438 = zext i32 %434 to i64
  call void @llvm.memset.p0i8.i64(i8* nonnull align 32 %89, i8 0, i64 800, i1 false)
  br label %for_begin25.preheader

for_end21:                                        ; preds = %for_end36
  ret void

for_begin34.preheader:                            ; preds = %for_end27
  store <8 x float> %491, <8 x float>* %.sub, align 32, !tbaa !209
  store <8 x float> %497, <8 x float>* %41, align 32, !tbaa !209
  store <8 x float> %503, <8 x float>* %43, align 32, !tbaa !209
  store <8 x float> %509, <8 x float>* %45, align 32, !tbaa !209
  store <8 x float> %515, <8 x float>* %47, align 32, !tbaa !209
  store <8 x float> %521, <8 x float>* %49, align 32, !tbaa !209
  store <8 x float> %527, <8 x float>* %51, align 32, !tbaa !209
  store <8 x float> %533, <8 x float>* %53, align 32, !tbaa !209
  store <8 x float> %539, <8 x float>* %55, align 32, !tbaa !209
  store <8 x float> %545, <8 x float>* %57, align 32, !tbaa !209
  store <8 x float> %551, <8 x float>* %59, align 32, !tbaa !209
  store <8 x float> %557, <8 x float>* %61, align 32, !tbaa !209
  store <8 x float> %563, <8 x float>* %63, align 32, !tbaa !209
  store <8 x float> %569, <8 x float>* %65, align 32, !tbaa !209
  store <8 x float> %575, <8 x float>* %67, align 32, !tbaa !209
  store <8 x float> %581, <8 x float>* %69, align 32, !tbaa !209
  store <8 x float> %587, <8 x float>* %71, align 32, !tbaa !209
  store <8 x float> %593, <8 x float>* %73, align 32, !tbaa !209
  store <8 x float> %599, <8 x float>* %75, align 32, !tbaa !209
  store <8 x float> %605, <8 x float>* %77, align 32, !tbaa !209
  store <8 x float> %611, <8 x float>* %79, align 32, !tbaa !209
  store <8 x float> %617, <8 x float>* %81, align 32, !tbaa !209
  store <8 x float> %623, <8 x float>* %83, align 32, !tbaa !209
  store <8 x float> %629, <8 x float>* %85, align 32, !tbaa !209
  store <8 x float> %635, <8 x float>* %87, align 32, !tbaa !209
  %439 = mul nuw nsw i32 %433, 25
  %440 = mul nsw i32 %435, 5000
  %441 = add nuw nsw i32 %440, %439
  %442 = zext i32 %441 to i64
  br label %for_body35

for_begin25.preheader:                            ; preds = %for_end27, %for_body20
  %indvars.iv226 = phi i64 [ 0, %for_body20 ], [ %indvars.iv.next227, %for_end27 ]
  %.lcssa49.lcssa.lcssa195 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %635, %for_end27 ]
  %.lcssa47.lcssa.lcssa193 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %629, %for_end27 ]
  %.lcssa45.lcssa.lcssa191 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %623, %for_end27 ]
  %.lcssa43.lcssa.lcssa189 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %617, %for_end27 ]
  %.lcssa41.lcssa.lcssa187 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %611, %for_end27 ]
  %.lcssa39.lcssa.lcssa185 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %605, %for_end27 ]
  %.lcssa37.lcssa.lcssa183 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %599, %for_end27 ]
  %.lcssa35.lcssa.lcssa181 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %593, %for_end27 ]
  %.lcssa33.lcssa.lcssa179 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %587, %for_end27 ]
  %.lcssa31.lcssa.lcssa177 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %581, %for_end27 ]
  %.lcssa29.lcssa.lcssa175 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %575, %for_end27 ]
  %.lcssa27.lcssa.lcssa173 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %569, %for_end27 ]
  %.lcssa25.lcssa.lcssa171 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %563, %for_end27 ]
  %.lcssa23.lcssa.lcssa169 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %557, %for_end27 ]
  %.lcssa21.lcssa.lcssa167 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %551, %for_end27 ]
  %.lcssa19.lcssa.lcssa165 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %545, %for_end27 ]
  %.lcssa17.lcssa.lcssa163 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %539, %for_end27 ]
  %.lcssa15.lcssa.lcssa161 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %533, %for_end27 ]
  %.lcssa13.lcssa.lcssa159 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %527, %for_end27 ]
  %.lcssa11.lcssa.lcssa157 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %521, %for_end27 ]
  %.lcssa9.lcssa.lcssa155 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %515, %for_end27 ]
  %.lcssa7.lcssa.lcssa154 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %509, %for_end27 ]
  %.lcssa5.lcssa.lcssa152 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %503, %for_end27 ]
  %.lcssa3.lcssa.lcssa150 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %497, %for_end27 ]
  %.lcssa.lcssa.lcssa148 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %491, %for_end27 ]
  %443 = mul nuw nsw i64 %indvars.iv226, 22472
  %444 = add nuw nsw i64 %443, %438
  %445 = mul nuw nsw i64 %indvars.iv226, 1600
  %446 = add nuw nsw i64 %445, %437
  br label %for_begin28.preheader

for_begin28.preheader:                            ; preds = %for_end30, %for_begin25.preheader
  %indvars.iv223 = phi i64 [ 0, %for_begin25.preheader ], [ %indvars.iv.next224, %for_end30 ]
  %.lcssa49.lcssa146 = phi <8 x float> [ %.lcssa49.lcssa.lcssa195, %for_begin25.preheader ], [ %635, %for_end30 ]
  %.lcssa47.lcssa144 = phi <8 x float> [ %.lcssa47.lcssa.lcssa193, %for_begin25.preheader ], [ %629, %for_end30 ]
  %.lcssa45.lcssa142 = phi <8 x float> [ %.lcssa45.lcssa.lcssa191, %for_begin25.preheader ], [ %623, %for_end30 ]
  %.lcssa43.lcssa140 = phi <8 x float> [ %.lcssa43.lcssa.lcssa189, %for_begin25.preheader ], [ %617, %for_end30 ]
  %.lcssa41.lcssa138 = phi <8 x float> [ %.lcssa41.lcssa.lcssa187, %for_begin25.preheader ], [ %611, %for_end30 ]
  %.lcssa39.lcssa136 = phi <8 x float> [ %.lcssa39.lcssa.lcssa185, %for_begin25.preheader ], [ %605, %for_end30 ]
  %.lcssa37.lcssa134 = phi <8 x float> [ %.lcssa37.lcssa.lcssa183, %for_begin25.preheader ], [ %599, %for_end30 ]
  %.lcssa35.lcssa132 = phi <8 x float> [ %.lcssa35.lcssa.lcssa181, %for_begin25.preheader ], [ %593, %for_end30 ]
  %.lcssa33.lcssa130 = phi <8 x float> [ %.lcssa33.lcssa.lcssa179, %for_begin25.preheader ], [ %587, %for_end30 ]
  %.lcssa31.lcssa128 = phi <8 x float> [ %.lcssa31.lcssa.lcssa177, %for_begin25.preheader ], [ %581, %for_end30 ]
  %.lcssa29.lcssa126 = phi <8 x float> [ %.lcssa29.lcssa.lcssa175, %for_begin25.preheader ], [ %575, %for_end30 ]
  %.lcssa27.lcssa124 = phi <8 x float> [ %.lcssa27.lcssa.lcssa173, %for_begin25.preheader ], [ %569, %for_end30 ]
  %.lcssa25.lcssa122 = phi <8 x float> [ %.lcssa25.lcssa.lcssa171, %for_begin25.preheader ], [ %563, %for_end30 ]
  %.lcssa23.lcssa120 = phi <8 x float> [ %.lcssa23.lcssa.lcssa169, %for_begin25.preheader ], [ %557, %for_end30 ]
  %.lcssa21.lcssa118 = phi <8 x float> [ %.lcssa21.lcssa.lcssa167, %for_begin25.preheader ], [ %551, %for_end30 ]
  %.lcssa19.lcssa116 = phi <8 x float> [ %.lcssa19.lcssa.lcssa165, %for_begin25.preheader ], [ %545, %for_end30 ]
  %.lcssa17.lcssa114 = phi <8 x float> [ %.lcssa17.lcssa.lcssa163, %for_begin25.preheader ], [ %539, %for_end30 ]
  %.lcssa15.lcssa112 = phi <8 x float> [ %.lcssa15.lcssa.lcssa161, %for_begin25.preheader ], [ %533, %for_end30 ]
  %.lcssa13.lcssa110 = phi <8 x float> [ %.lcssa13.lcssa.lcssa159, %for_begin25.preheader ], [ %527, %for_end30 ]
  %.lcssa11.lcssa108 = phi <8 x float> [ %.lcssa11.lcssa.lcssa157, %for_begin25.preheader ], [ %521, %for_end30 ]
  %.lcssa9.lcssa106 = phi <8 x float> [ %.lcssa9.lcssa.lcssa155, %for_begin25.preheader ], [ %515, %for_end30 ]
  %.lcssa7.lcssa104 = phi <8 x float> [ %.lcssa7.lcssa.lcssa154, %for_begin25.preheader ], [ %509, %for_end30 ]
  %.lcssa5.lcssa103 = phi <8 x float> [ %.lcssa5.lcssa.lcssa152, %for_begin25.preheader ], [ %503, %for_end30 ]
  %.lcssa3.lcssa101 = phi <8 x float> [ %.lcssa3.lcssa.lcssa150, %for_begin25.preheader ], [ %497, %for_end30 ]
  %.lcssa.lcssa99 = phi <8 x float> [ %.lcssa.lcssa.lcssa148, %for_begin25.preheader ], [ %491, %for_end30 ]
  %447 = mul nuw nsw i64 %indvars.iv223, 424
  %448 = add nuw nsw i64 %444, %447
  %449 = mul nuw nsw i64 %indvars.iv223, 320
  %450 = add nuw nsw i64 %446, %449
  br label %for_begin31.preheader

for_end27:                                        ; preds = %for_end30
  %indvars.iv.next227 = add nuw nsw i64 %indvars.iv226, 1
  %exitcond228 = icmp eq i64 %indvars.iv.next227, 12
  br i1 %exitcond228, label %for_begin34.preheader, label %for_begin25.preheader, !prof !28

for_begin31.preheader:                            ; preds = %for_end33, %for_begin28.preheader
  %indvars.iv220 = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next221, %for_end33 ]
  %.lcssa4998 = phi <8 x float> [ %.lcssa49.lcssa146, %for_begin28.preheader ], [ %635, %for_end33 ]
  %.lcssa4796 = phi <8 x float> [ %.lcssa47.lcssa144, %for_begin28.preheader ], [ %629, %for_end33 ]
  %.lcssa4594 = phi <8 x float> [ %.lcssa45.lcssa142, %for_begin28.preheader ], [ %623, %for_end33 ]
  %.lcssa4392 = phi <8 x float> [ %.lcssa43.lcssa140, %for_begin28.preheader ], [ %617, %for_end33 ]
  %.lcssa4190 = phi <8 x float> [ %.lcssa41.lcssa138, %for_begin28.preheader ], [ %611, %for_end33 ]
  %.lcssa3988 = phi <8 x float> [ %.lcssa39.lcssa136, %for_begin28.preheader ], [ %605, %for_end33 ]
  %.lcssa3786 = phi <8 x float> [ %.lcssa37.lcssa134, %for_begin28.preheader ], [ %599, %for_end33 ]
  %.lcssa3584 = phi <8 x float> [ %.lcssa35.lcssa132, %for_begin28.preheader ], [ %593, %for_end33 ]
  %.lcssa3382 = phi <8 x float> [ %.lcssa33.lcssa130, %for_begin28.preheader ], [ %587, %for_end33 ]
  %.lcssa3180 = phi <8 x float> [ %.lcssa31.lcssa128, %for_begin28.preheader ], [ %581, %for_end33 ]
  %.lcssa2978 = phi <8 x float> [ %.lcssa29.lcssa126, %for_begin28.preheader ], [ %575, %for_end33 ]
  %.lcssa2776 = phi <8 x float> [ %.lcssa27.lcssa124, %for_begin28.preheader ], [ %569, %for_end33 ]
  %.lcssa2574 = phi <8 x float> [ %.lcssa25.lcssa122, %for_begin28.preheader ], [ %563, %for_end33 ]
  %.lcssa2372 = phi <8 x float> [ %.lcssa23.lcssa120, %for_begin28.preheader ], [ %557, %for_end33 ]
  %.lcssa2170 = phi <8 x float> [ %.lcssa21.lcssa118, %for_begin28.preheader ], [ %551, %for_end33 ]
  %.lcssa1968 = phi <8 x float> [ %.lcssa19.lcssa116, %for_begin28.preheader ], [ %545, %for_end33 ]
  %.lcssa1766 = phi <8 x float> [ %.lcssa17.lcssa114, %for_begin28.preheader ], [ %539, %for_end33 ]
  %.lcssa1564 = phi <8 x float> [ %.lcssa15.lcssa112, %for_begin28.preheader ], [ %533, %for_end33 ]
  %.lcssa1362 = phi <8 x float> [ %.lcssa13.lcssa110, %for_begin28.preheader ], [ %527, %for_end33 ]
  %.lcssa1160 = phi <8 x float> [ %.lcssa11.lcssa108, %for_begin28.preheader ], [ %521, %for_end33 ]
  %.lcssa958 = phi <8 x float> [ %.lcssa9.lcssa106, %for_begin28.preheader ], [ %515, %for_end33 ]
  %.lcssa756 = phi <8 x float> [ %.lcssa7.lcssa104, %for_begin28.preheader ], [ %509, %for_end33 ]
  %.lcssa554 = phi <8 x float> [ %.lcssa5.lcssa103, %for_begin28.preheader ], [ %503, %for_end33 ]
  %.lcssa353 = phi <8 x float> [ %.lcssa3.lcssa101, %for_begin28.preheader ], [ %497, %for_end33 ]
  %.lcssa51 = phi <8 x float> [ %.lcssa.lcssa99, %for_begin28.preheader ], [ %491, %for_end33 ]
  %451 = add nuw nsw i64 %448, %indvars.iv220
  %452 = shl i64 %indvars.iv220, 6
  %453 = add nuw nsw i64 %450, %452
  br label %for_body32

for_end30:                                        ; preds = %for_end33
  %indvars.iv.next224 = add nuw nsw i64 %indvars.iv223, 1
  %exitcond225 = icmp eq i64 %indvars.iv.next224, 5
  br i1 %exitcond225, label %for_end27, label %for_begin28.preheader, !prof !28

for_body32:                                       ; preds = %for_body32, %for_begin31.preheader
  %indvars.iv = phi i64 [ 0, %for_begin31.preheader ], [ %indvars.iv.next, %for_body32 ]
  %454 = phi <8 x float> [ %.lcssa4998, %for_begin31.preheader ], [ %635, %for_body32 ]
  %455 = phi <8 x float> [ %.lcssa4796, %for_begin31.preheader ], [ %629, %for_body32 ]
  %456 = phi <8 x float> [ %.lcssa4594, %for_begin31.preheader ], [ %623, %for_body32 ]
  %457 = phi <8 x float> [ %.lcssa4392, %for_begin31.preheader ], [ %617, %for_body32 ]
  %458 = phi <8 x float> [ %.lcssa4190, %for_begin31.preheader ], [ %611, %for_body32 ]
  %459 = phi <8 x float> [ %.lcssa3988, %for_begin31.preheader ], [ %605, %for_body32 ]
  %460 = phi <8 x float> [ %.lcssa3786, %for_begin31.preheader ], [ %599, %for_body32 ]
  %461 = phi <8 x float> [ %.lcssa3584, %for_begin31.preheader ], [ %593, %for_body32 ]
  %462 = phi <8 x float> [ %.lcssa3382, %for_begin31.preheader ], [ %587, %for_body32 ]
  %463 = phi <8 x float> [ %.lcssa3180, %for_begin31.preheader ], [ %581, %for_body32 ]
  %464 = phi <8 x float> [ %.lcssa2978, %for_begin31.preheader ], [ %575, %for_body32 ]
  %465 = phi <8 x float> [ %.lcssa2776, %for_begin31.preheader ], [ %569, %for_body32 ]
  %466 = phi <8 x float> [ %.lcssa2574, %for_begin31.preheader ], [ %563, %for_body32 ]
  %467 = phi <8 x float> [ %.lcssa2372, %for_begin31.preheader ], [ %557, %for_body32 ]
  %468 = phi <8 x float> [ %.lcssa2170, %for_begin31.preheader ], [ %551, %for_body32 ]
  %469 = phi <8 x float> [ %.lcssa1968, %for_begin31.preheader ], [ %545, %for_body32 ]
  %470 = phi <8 x float> [ %.lcssa1766, %for_begin31.preheader ], [ %539, %for_body32 ]
  %471 = phi <8 x float> [ %.lcssa1564, %for_begin31.preheader ], [ %533, %for_body32 ]
  %472 = phi <8 x float> [ %.lcssa1362, %for_begin31.preheader ], [ %527, %for_body32 ]
  %473 = phi <8 x float> [ %.lcssa1160, %for_begin31.preheader ], [ %521, %for_body32 ]
  %474 = phi <8 x float> [ %.lcssa958, %for_begin31.preheader ], [ %515, %for_body32 ]
  %475 = phi <8 x float> [ %.lcssa756, %for_begin31.preheader ], [ %509, %for_body32 ]
  %476 = phi <8 x float> [ %.lcssa554, %for_begin31.preheader ], [ %503, %for_body32 ]
  %477 = phi <8 x float> [ %.lcssa353, %for_begin31.preheader ], [ %497, %for_body32 ]
  %478 = phi <8 x float> [ %.lcssa51, %for_begin31.preheader ], [ %491, %for_body32 ]
  %479 = mul nuw nsw i64 %indvars.iv, 53
  %480 = add nuw nsw i64 %451, %479
  %481 = and i64 %480, 4294967295
  %482 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %481
  %483 = load float, float* %482, align 4, !tbaa !220
  %484 = insertelement <8 x float> undef, float %483, i32 0
  %485 = shufflevector <8 x float> %484, <8 x float> undef, <8 x i32> zeroinitializer
  %486 = shl i64 %indvars.iv, 3
  %487 = add nuw nsw i64 %453, %486
  %488 = getelementptr inbounds [76800 x <8 x float>], [76800 x <8 x float>]* %4, i64 0, i64 0, i64 %487
  %489 = bitcast float* %488 to <8 x float>*
  %490 = load <8 x float>, <8 x float>* %489, align 16, !tbaa !206
  %491 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %485, <8 x float> %490, <8 x float> %478)
  %492 = add nuw nsw i64 %480, 2
  %493 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %492
  %494 = load float, float* %493, align 4, !tbaa !220
  %495 = insertelement <8 x float> undef, float %494, i32 0
  %496 = shufflevector <8 x float> %495, <8 x float> undef, <8 x i32> zeroinitializer
  %497 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %496, <8 x float> %490, <8 x float> %477)
  %498 = add nuw nsw i64 %480, 4
  %499 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %498
  %500 = load float, float* %499, align 4, !tbaa !220
  %501 = insertelement <8 x float> undef, float %500, i32 0
  %502 = shufflevector <8 x float> %501, <8 x float> undef, <8 x i32> zeroinitializer
  %503 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %502, <8 x float> %490, <8 x float> %476)
  %504 = add nuw nsw i64 %480, 6
  %505 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %504
  %506 = load float, float* %505, align 4, !tbaa !220
  %507 = insertelement <8 x float> undef, float %506, i32 0
  %508 = shufflevector <8 x float> %507, <8 x float> undef, <8 x i32> zeroinitializer
  %509 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %508, <8 x float> %490, <8 x float> %475)
  %510 = add nuw nsw i64 %480, 8
  %511 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %510
  %512 = load float, float* %511, align 4, !tbaa !220
  %513 = insertelement <8 x float> undef, float %512, i32 0
  %514 = shufflevector <8 x float> %513, <8 x float> undef, <8 x i32> zeroinitializer
  %515 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %514, <8 x float> %490, <8 x float> %474)
  %516 = add nuw nsw i64 %480, 10
  %517 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %516
  %518 = load float, float* %517, align 4, !tbaa !220
  %519 = insertelement <8 x float> undef, float %518, i32 0
  %520 = shufflevector <8 x float> %519, <8 x float> undef, <8 x i32> zeroinitializer
  %521 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %520, <8 x float> %490, <8 x float> %473)
  %522 = add nuw nsw i64 %480, 12
  %523 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %522
  %524 = load float, float* %523, align 4, !tbaa !220
  %525 = insertelement <8 x float> undef, float %524, i32 0
  %526 = shufflevector <8 x float> %525, <8 x float> undef, <8 x i32> zeroinitializer
  %527 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %526, <8 x float> %490, <8 x float> %472)
  %528 = add nuw nsw i64 %480, 14
  %529 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %528
  %530 = load float, float* %529, align 4, !tbaa !220
  %531 = insertelement <8 x float> undef, float %530, i32 0
  %532 = shufflevector <8 x float> %531, <8 x float> undef, <8 x i32> zeroinitializer
  %533 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %532, <8 x float> %490, <8 x float> %471)
  %534 = add nuw nsw i64 %480, 16
  %535 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %534
  %536 = load float, float* %535, align 4, !tbaa !220
  %537 = insertelement <8 x float> undef, float %536, i32 0
  %538 = shufflevector <8 x float> %537, <8 x float> undef, <8 x i32> zeroinitializer
  %539 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %538, <8 x float> %490, <8 x float> %470)
  %540 = add nuw nsw i64 %480, 18
  %541 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %540
  %542 = load float, float* %541, align 4, !tbaa !220
  %543 = insertelement <8 x float> undef, float %542, i32 0
  %544 = shufflevector <8 x float> %543, <8 x float> undef, <8 x i32> zeroinitializer
  %545 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %544, <8 x float> %490, <8 x float> %469)
  %546 = add nuw nsw i64 %480, 20
  %547 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %546
  %548 = load float, float* %547, align 4, !tbaa !220
  %549 = insertelement <8 x float> undef, float %548, i32 0
  %550 = shufflevector <8 x float> %549, <8 x float> undef, <8 x i32> zeroinitializer
  %551 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %550, <8 x float> %490, <8 x float> %468)
  %552 = add nuw nsw i64 %480, 22
  %553 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %552
  %554 = load float, float* %553, align 4, !tbaa !220
  %555 = insertelement <8 x float> undef, float %554, i32 0
  %556 = shufflevector <8 x float> %555, <8 x float> undef, <8 x i32> zeroinitializer
  %557 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %556, <8 x float> %490, <8 x float> %467)
  %558 = add nuw nsw i64 %480, 24
  %559 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %558
  %560 = load float, float* %559, align 4, !tbaa !220
  %561 = insertelement <8 x float> undef, float %560, i32 0
  %562 = shufflevector <8 x float> %561, <8 x float> undef, <8 x i32> zeroinitializer
  %563 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %562, <8 x float> %490, <8 x float> %466)
  %564 = add nuw nsw i64 %480, 26
  %565 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %564
  %566 = load float, float* %565, align 4, !tbaa !220
  %567 = insertelement <8 x float> undef, float %566, i32 0
  %568 = shufflevector <8 x float> %567, <8 x float> undef, <8 x i32> zeroinitializer
  %569 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %568, <8 x float> %490, <8 x float> %465)
  %570 = add nuw nsw i64 %480, 28
  %571 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %570
  %572 = load float, float* %571, align 4, !tbaa !220
  %573 = insertelement <8 x float> undef, float %572, i32 0
  %574 = shufflevector <8 x float> %573, <8 x float> undef, <8 x i32> zeroinitializer
  %575 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %574, <8 x float> %490, <8 x float> %464)
  %576 = add nuw nsw i64 %480, 30
  %577 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %576
  %578 = load float, float* %577, align 4, !tbaa !220
  %579 = insertelement <8 x float> undef, float %578, i32 0
  %580 = shufflevector <8 x float> %579, <8 x float> undef, <8 x i32> zeroinitializer
  %581 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %580, <8 x float> %490, <8 x float> %463)
  %582 = add nuw nsw i64 %480, 32
  %583 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %582
  %584 = load float, float* %583, align 4, !tbaa !220
  %585 = insertelement <8 x float> undef, float %584, i32 0
  %586 = shufflevector <8 x float> %585, <8 x float> undef, <8 x i32> zeroinitializer
  %587 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %586, <8 x float> %490, <8 x float> %462)
  %588 = add nuw nsw i64 %480, 34
  %589 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %588
  %590 = load float, float* %589, align 4, !tbaa !220
  %591 = insertelement <8 x float> undef, float %590, i32 0
  %592 = shufflevector <8 x float> %591, <8 x float> undef, <8 x i32> zeroinitializer
  %593 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %592, <8 x float> %490, <8 x float> %461)
  %594 = add nuw nsw i64 %480, 36
  %595 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %594
  %596 = load float, float* %595, align 4, !tbaa !220
  %597 = insertelement <8 x float> undef, float %596, i32 0
  %598 = shufflevector <8 x float> %597, <8 x float> undef, <8 x i32> zeroinitializer
  %599 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %598, <8 x float> %490, <8 x float> %460)
  %600 = add nuw nsw i64 %480, 38
  %601 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %600
  %602 = load float, float* %601, align 4, !tbaa !220
  %603 = insertelement <8 x float> undef, float %602, i32 0
  %604 = shufflevector <8 x float> %603, <8 x float> undef, <8 x i32> zeroinitializer
  %605 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %604, <8 x float> %490, <8 x float> %459)
  %606 = add nuw nsw i64 %480, 40
  %607 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %606
  %608 = load float, float* %607, align 4, !tbaa !220
  %609 = insertelement <8 x float> undef, float %608, i32 0
  %610 = shufflevector <8 x float> %609, <8 x float> undef, <8 x i32> zeroinitializer
  %611 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %610, <8 x float> %490, <8 x float> %458)
  %612 = add nuw nsw i64 %480, 42
  %613 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %612
  %614 = load float, float* %613, align 4, !tbaa !220
  %615 = insertelement <8 x float> undef, float %614, i32 0
  %616 = shufflevector <8 x float> %615, <8 x float> undef, <8 x i32> zeroinitializer
  %617 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %616, <8 x float> %490, <8 x float> %457)
  %618 = add nuw nsw i64 %480, 44
  %619 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %618
  %620 = load float, float* %619, align 4, !tbaa !220
  %621 = insertelement <8 x float> undef, float %620, i32 0
  %622 = shufflevector <8 x float> %621, <8 x float> undef, <8 x i32> zeroinitializer
  %623 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %622, <8 x float> %490, <8 x float> %456)
  %624 = add nuw nsw i64 %480, 46
  %625 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %624
  %626 = load float, float* %625, align 4, !tbaa !220
  %627 = insertelement <8 x float> undef, float %626, i32 0
  %628 = shufflevector <8 x float> %627, <8 x float> undef, <8 x i32> zeroinitializer
  %629 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %628, <8 x float> %490, <8 x float> %455)
  %630 = add nuw nsw i64 %480, 48
  %631 = getelementptr inbounds [269664 x float], [269664 x float]* %5, i64 0, i64 %630
  %632 = load float, float* %631, align 4, !tbaa !220
  %633 = insertelement <8 x float> undef, float %632, i32 0
  %634 = shufflevector <8 x float> %633, <8 x float> undef, <8 x i32> zeroinitializer
  %635 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %634, <8 x float> %490, <8 x float> %454)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 8
  br i1 %exitcond, label %for_end33, label %for_body32, !prof !28

for_end33:                                        ; preds = %for_body32
  %indvars.iv.next221 = add nuw nsw i64 %indvars.iv220, 1
  %exitcond222 = icmp eq i64 %indvars.iv.next221, 5
  br i1 %exitcond222, label %for_end30, label %for_begin31.preheader, !prof !28

for_body35:                                       ; preds = %for_body35, %for_begin34.preheader
  %indvars.iv229 = phi i64 [ 0, %for_begin34.preheader ], [ %indvars.iv.next230, %for_body35 ]
  %636 = add nuw nsw i64 %indvars.iv229, %442
  %637 = add nuw nsw i64 %636, 625
  %638 = add nuw nsw i64 %636, 1250
  %639 = add nuw nsw i64 %636, 1875
  %640 = add nuw nsw i64 %636, 2500
  %641 = add nuw nsw i64 %636, 3125
  %642 = add nuw nsw i64 %636, 3750
  %643 = add nuw nsw i64 %636, 4375
  %644 = shl nsw i64 %indvars.iv229, 3
  %645 = getelementptr inbounds [25 x <8 x float>], [25 x <8 x float>]* %3, i64 0, i64 0, i64 %644
  %646 = bitcast float* %645 to <8 x float>*
  %647 = load <8 x float>, <8 x float>* %646, align 32, !tbaa !223
  %648 = getelementptr inbounds float, float* %88, i64 %636
  %649 = extractelement <8 x float> %647, i64 0
  store float %649, float* %648, align 4, !tbaa !224
  %650 = getelementptr inbounds float, float* %88, i64 %637
  %651 = extractelement <8 x float> %647, i64 1
  store float %651, float* %650, align 4, !tbaa !224
  %652 = getelementptr inbounds float, float* %88, i64 %638
  %653 = extractelement <8 x float> %647, i64 2
  store float %653, float* %652, align 4, !tbaa !224
  %654 = getelementptr inbounds float, float* %88, i64 %639
  %655 = extractelement <8 x float> %647, i64 3
  store float %655, float* %654, align 4, !tbaa !224
  %656 = getelementptr inbounds float, float* %88, i64 %640
  %657 = extractelement <8 x float> %647, i64 4
  store float %657, float* %656, align 4, !tbaa !224
  %658 = getelementptr inbounds float, float* %88, i64 %641
  %659 = extractelement <8 x float> %647, i64 5
  store float %659, float* %658, align 4, !tbaa !224
  %660 = getelementptr inbounds float, float* %88, i64 %642
  %661 = extractelement <8 x float> %647, i64 6
  store float %661, float* %660, align 4, !tbaa !224
  %662 = getelementptr inbounds float, float* %88, i64 %643
  %663 = extractelement <8 x float> %647, i64 7
  store float %663, float* %662, align 4, !tbaa !224
  %indvars.iv.next230 = add nuw nsw i64 %indvars.iv229, 1
  %exitcond231 = icmp eq i64 %indvars.iv.next230, 25
  br i1 %exitcond231, label %for_end36, label %for_body35, !prof !28

for_end36:                                        ; preds = %for_body35
  %664 = add nuw nsw i32 %432, 1
  %exitcond232 = icmp eq i32 %664, 800
  br i1 %exitcond232, label %for_end21, label %for_body20, !prof !28
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_multiply(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_multiply_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_multiply_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = load float, float* %4, align 64, !tbaa !227
  %6 = bitcast i8* %0 to float*
  %broadcast.splatinsert3 = insertelement <4 x float> undef, float %5, i32 0
  %broadcast.splat4 = shufflevector <4 x float> %broadcast.splatinsert3, <4 x float> undef, <4 x i32> zeroinitializer
  %broadcast.splatinsert5 = insertelement <4 x float> undef, float %5, i32 0
  %broadcast.splat6 = shufflevector <4 x float> %broadcast.splatinsert5, <4 x float> undef, <4 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ]
  %7 = getelementptr inbounds float, float* %3, i64 %index
  %8 = bitcast float* %7 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !241
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !241
  %11 = fmul <4 x float> %wide.load, %broadcast.splat4
  %12 = fmul <4 x float> %wide.load2, %broadcast.splat6
  %13 = getelementptr inbounds float, float* %6, i64 %index
  %14 = bitcast float* %13 to <4 x float>*
  store <4 x float> %11, <4 x float>* %14, align 4, !tbaa !244
  %15 = getelementptr inbounds float, float* %13, i64 4
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %12, <4 x float>* %16, align 4, !tbaa !244
  %index.next = add i64 %index, 8
  %17 = icmp eq i64 %index.next, 1024
  br i1 %17, label %for_end, label %vector.body, !llvm.loop !247

for_end:                                          ; preds = %vector.body
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_bias_add(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_bias_add_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_bias_add_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %0 to float*
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ]
  %6 = getelementptr inbounds float, float* %3, i64 %index
  %7 = bitcast float* %6 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %7, align 4, !tbaa !248
  %8 = getelementptr inbounds float, float* %6, i64 4
  %9 = bitcast float* %8 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !248
  %10 = getelementptr inbounds float, float* %4, i64 %index
  %11 = bitcast float* %10 to <4 x float>*
  %wide.load3 = load <4 x float>, <4 x float>* %11, align 4, !tbaa !251
  %12 = getelementptr inbounds float, float* %10, i64 4
  %13 = bitcast float* %12 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %13, align 4, !tbaa !251
  %14 = fadd <4 x float> %wide.load, %wide.load3
  %15 = fadd <4 x float> %wide.load2, %wide.load4
  %16 = getelementptr inbounds float, float* %5, i64 %index
  %17 = bitcast float* %16 to <4 x float>*
  store <4 x float> %14, <4 x float>* %17, align 4, !tbaa !254
  %18 = getelementptr inbounds float, float* %16, i64 4
  %19 = bitcast float* %18 to <4 x float>*
  store <4 x float> %15, <4 x float>* %19, align 4, !tbaa !254
  %index.next = add i64 %index, 8
  %20 = icmp eq i64 %index.next, 1000
  br i1 %20, label %for_end, label %vector.body, !llvm.loop !257

for_end:                                          ; preds = %vector.body
  ret void
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_dense(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
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
  tail call fastcc void @fused_nn_dense_compute_(i8* %12, i8* %14, i8* %16)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_dense_compute_(i8* noalias nocapture readonly, i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #3 {
entry:
  %3 = bitcast i8* %0 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %2 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv3 = phi i64 [ 0, %entry ], [ %indvars.iv.next4, %for_end3 ]
  %6 = shl i64 %indvars.iv3, 10
  br label %for_body2

for_end:                                          ; preds = %for_end3
  ret void

for_body2:                                        ; preds = %for_body2, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_body2 ]
  %.02 = phi <16 x float> [ zeroinitializer, %for_begin1.preheader ], [ %15, %for_body2 ]
  %7 = shl nsw i64 %indvars.iv, 4
  %8 = getelementptr inbounds float, float* %3, i64 %7
  %9 = bitcast float* %8 to <16 x float>*
  %10 = load <16 x float>, <16 x float>* %9, align 64, !tbaa !258
  %11 = add nuw nsw i64 %7, %6
  %12 = getelementptr inbounds float, float* %4, i64 %11
  %13 = bitcast float* %12 to <16 x float>*
  %14 = load <16 x float>, <16 x float>* %13, align 64, !tbaa !261
  %15 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %10, <16 x float> %14, <16 x float> %.02)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 64
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2
  %16 = getelementptr inbounds float, float* %5, i64 %indvars.iv3
  %.0.vec.extract = extractelement <16 x float> %15, i32 0
  %17 = fadd float %.0.vec.extract, 0.000000e+00
  %.4.vec.extract = extractelement <16 x float> %15, i32 1
  %18 = fadd float %.4.vec.extract, %17
  %.8.vec.extract = extractelement <16 x float> %15, i32 2
  %19 = fadd float %.8.vec.extract, %18
  %.12.vec.extract = extractelement <16 x float> %15, i32 3
  %20 = fadd float %.12.vec.extract, %19
  %.16.vec.extract = extractelement <16 x float> %15, i32 4
  %21 = fadd float %.16.vec.extract, %20
  %.20.vec.extract = extractelement <16 x float> %15, i32 5
  %22 = fadd float %.20.vec.extract, %21
  %.24.vec.extract = extractelement <16 x float> %15, i32 6
  %23 = fadd float %.24.vec.extract, %22
  %.28.vec.extract = extractelement <16 x float> %15, i32 7
  %24 = fadd float %.28.vec.extract, %23
  %.32.vec.extract = extractelement <16 x float> %15, i32 8
  %25 = fadd float %.32.vec.extract, %24
  %.36.vec.extract = extractelement <16 x float> %15, i32 9
  %26 = fadd float %.36.vec.extract, %25
  %.40.vec.extract = extractelement <16 x float> %15, i32 10
  %27 = fadd float %.40.vec.extract, %26
  %.44.vec.extract = extractelement <16 x float> %15, i32 11
  %28 = fadd float %.44.vec.extract, %27
  %.48.vec.extract = extractelement <16 x float> %15, i32 12
  %29 = fadd float %.48.vec.extract, %28
  %.52.vec.extract = extractelement <16 x float> %15, i32 13
  %30 = fadd float %.52.vec.extract, %29
  %.56.vec.extract = extractelement <16 x float> %15, i32 14
  %31 = fadd float %.56.vec.extract, %30
  %.60.vec.extract = extractelement <16 x float> %15, i32 15
  %32 = fadd float %.60.vec.extract, %31
  store float %32, float* %16, align 4, !tbaa !264
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 1000
  br i1 %exitcond5, label %for_end, label %for_begin1.preheader, !prof !28
}

; Function Attrs: nounwind readnone speculatable
declare <16 x float> @llvm.fmuladd.v16f32(<16 x float>, <16 x float>, <16 x float>) #4

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
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 73728, i1 false)
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_batch_flatten(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_batch_flatten_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_batch_flatten_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 4096, i1 false)
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_bias_add_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_bias_add_1_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_bias_add_1_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %0 to float*
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ]
  %6 = getelementptr inbounds float, float* %3, i64 %index
  %7 = bitcast float* %6 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %7, align 4, !tbaa !267
  %8 = getelementptr inbounds float, float* %6, i64 4
  %9 = bitcast float* %8 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !267
  %10 = getelementptr inbounds float, float* %4, i64 %index
  %11 = bitcast float* %10 to <4 x float>*
  %wide.load3 = load <4 x float>, <4 x float>* %11, align 4, !tbaa !270
  %12 = getelementptr inbounds float, float* %10, i64 4
  %13 = bitcast float* %12 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %13, align 4, !tbaa !270
  %14 = fadd <4 x float> %wide.load, %wide.load3
  %15 = fadd <4 x float> %wide.load2, %wide.load4
  %16 = getelementptr inbounds float, float* %5, i64 %index
  %17 = bitcast float* %16 to <4 x float>*
  store <4 x float> %14, <4 x float>* %17, align 4, !tbaa !273
  %18 = getelementptr inbounds float, float* %16, i64 4
  %19 = bitcast float* %18 to <4 x float>*
  store <4 x float> %15, <4 x float>* %19, align 4, !tbaa !273
  %index.next = add i64 %index, 8
  %20 = icmp eq i64 %index.next, 1024
  br i1 %20, label %for_end, label %vector.body, !llvm.loop !276

for_end:                                          ; preds = %vector.body
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_batch_flatten_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_batch_flatten_2_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_batch_flatten_2_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 73728, i1 false)
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_multiply_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_multiply_1_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_multiply_1_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = load float, float* %4, align 64, !tbaa !277
  %6 = bitcast i8* %0 to float*
  %broadcast.splatinsert3 = insertelement <4 x float> undef, float %5, i32 0
  %broadcast.splat4 = shufflevector <4 x float> %broadcast.splatinsert3, <4 x float> undef, <4 x i32> zeroinitializer
  %broadcast.splatinsert5 = insertelement <4 x float> undef, float %5, i32 0
  %broadcast.splat6 = shufflevector <4 x float> %broadcast.splatinsert5, <4 x float> undef, <4 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ]
  %7 = getelementptr inbounds float, float* %3, i64 %index
  %8 = bitcast float* %7 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !291
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !291
  %11 = fmul <4 x float> %wide.load, %broadcast.splat4
  %12 = fmul <4 x float> %wide.load2, %broadcast.splat6
  %13 = getelementptr inbounds float, float* %6, i64 %index
  %14 = bitcast float* %13 to <4 x float>*
  store <4 x float> %11, <4 x float>* %14, align 4, !tbaa !294
  %15 = getelementptr inbounds float, float* %13, i64 4
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %12, <4 x float>* %16, align 4, !tbaa !294
  %index.next = add i64 %index, 8
  %17 = icmp eq i64 %index.next, 4096
  br i1 %17, label %for_end, label %vector.body, !llvm.loop !297

for_end:                                          ; preds = %vector.body
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_batch_flatten_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_batch_flatten_1_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_batch_flatten_1_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 16384, i1 false)
  ret void
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

for_begin1.preheader:                             ; preds = %for_begin1.preheader, %entry
  %indvars.iv1 = phi i64 [ 0, %entry ], [ %indvars.iv.next2, %for_begin1.preheader ]
  %4 = mul nuw nsw i64 %indvars.iv1, 144
  %5 = getelementptr inbounds float, float* %2, i64 %4
  %6 = getelementptr inbounds float, float* %3, i64 %4
  %7 = bitcast float* %5 to <4 x float>*
  %8 = load <4 x float>, <4 x float>* %7, align 4, !tbaa !298
  %9 = fcmp ogt <4 x float> %8, zeroinitializer
  %10 = select <4 x i1> %9, <4 x float> %8, <4 x float> zeroinitializer
  %11 = bitcast float* %6 to <4 x float>*
  store <4 x float> %10, <4 x float>* %11, align 4, !tbaa !301
  %12 = or i64 %4, 4
  %13 = getelementptr inbounds float, float* %2, i64 %12
  %14 = getelementptr inbounds float, float* %3, i64 %12
  %15 = bitcast float* %13 to <4 x float>*
  %16 = load <4 x float>, <4 x float>* %15, align 4, !tbaa !298
  %17 = fcmp ogt <4 x float> %16, zeroinitializer
  %18 = select <4 x i1> %17, <4 x float> %16, <4 x float> zeroinitializer
  %19 = bitcast float* %14 to <4 x float>*
  store <4 x float> %18, <4 x float>* %19, align 4, !tbaa !301
  %20 = or i64 %4, 8
  %21 = getelementptr inbounds float, float* %2, i64 %20
  %22 = getelementptr inbounds float, float* %3, i64 %20
  %23 = bitcast float* %21 to <4 x float>*
  %24 = load <4 x float>, <4 x float>* %23, align 4, !tbaa !298
  %25 = fcmp ogt <4 x float> %24, zeroinitializer
  %26 = select <4 x i1> %25, <4 x float> %24, <4 x float> zeroinitializer
  %27 = bitcast float* %22 to <4 x float>*
  store <4 x float> %26, <4 x float>* %27, align 4, !tbaa !301
  %28 = or i64 %4, 12
  %29 = getelementptr inbounds float, float* %2, i64 %28
  %30 = getelementptr inbounds float, float* %3, i64 %28
  %31 = bitcast float* %29 to <4 x float>*
  %32 = load <4 x float>, <4 x float>* %31, align 4, !tbaa !298
  %33 = fcmp ogt <4 x float> %32, zeroinitializer
  %34 = select <4 x i1> %33, <4 x float> %32, <4 x float> zeroinitializer
  %35 = bitcast float* %30 to <4 x float>*
  store <4 x float> %34, <4 x float>* %35, align 4, !tbaa !301
  %36 = add nuw nsw i64 %28, 4
  %37 = getelementptr inbounds float, float* %2, i64 %36
  %38 = getelementptr inbounds float, float* %3, i64 %36
  %39 = bitcast float* %37 to <4 x float>*
  %40 = load <4 x float>, <4 x float>* %39, align 4, !tbaa !298
  %41 = fcmp ogt <4 x float> %40, zeroinitializer
  %42 = select <4 x i1> %41, <4 x float> %40, <4 x float> zeroinitializer
  %43 = bitcast float* %38 to <4 x float>*
  store <4 x float> %42, <4 x float>* %43, align 4, !tbaa !301
  %44 = add nuw nsw i64 %28, 8
  %45 = getelementptr inbounds float, float* %2, i64 %44
  %46 = getelementptr inbounds float, float* %3, i64 %44
  %47 = bitcast float* %45 to <4 x float>*
  %48 = load <4 x float>, <4 x float>* %47, align 4, !tbaa !298
  %49 = fcmp ogt <4 x float> %48, zeroinitializer
  %50 = select <4 x i1> %49, <4 x float> %48, <4 x float> zeroinitializer
  %51 = bitcast float* %46 to <4 x float>*
  store <4 x float> %50, <4 x float>* %51, align 4, !tbaa !301
  %52 = add nuw nsw i64 %4, 24
  %53 = getelementptr inbounds float, float* %2, i64 %52
  %54 = getelementptr inbounds float, float* %3, i64 %52
  %55 = bitcast float* %53 to <4 x float>*
  %56 = load <4 x float>, <4 x float>* %55, align 4, !tbaa !298
  %57 = fcmp ogt <4 x float> %56, zeroinitializer
  %58 = select <4 x i1> %57, <4 x float> %56, <4 x float> zeroinitializer
  %59 = bitcast float* %54 to <4 x float>*
  store <4 x float> %58, <4 x float>* %59, align 4, !tbaa !301
  %60 = add nuw nsw i64 %4, 28
  %61 = getelementptr inbounds float, float* %2, i64 %60
  %62 = getelementptr inbounds float, float* %3, i64 %60
  %63 = bitcast float* %61 to <4 x float>*
  %64 = load <4 x float>, <4 x float>* %63, align 4, !tbaa !298
  %65 = fcmp ogt <4 x float> %64, zeroinitializer
  %66 = select <4 x i1> %65, <4 x float> %64, <4 x float> zeroinitializer
  %67 = bitcast float* %62 to <4 x float>*
  store <4 x float> %66, <4 x float>* %67, align 4, !tbaa !301
  %68 = add nuw nsw i64 %4, 32
  %69 = getelementptr inbounds float, float* %2, i64 %68
  %70 = getelementptr inbounds float, float* %3, i64 %68
  %71 = bitcast float* %69 to <4 x float>*
  %72 = load <4 x float>, <4 x float>* %71, align 4, !tbaa !298
  %73 = fcmp ogt <4 x float> %72, zeroinitializer
  %74 = select <4 x i1> %73, <4 x float> %72, <4 x float> zeroinitializer
  %75 = bitcast float* %70 to <4 x float>*
  store <4 x float> %74, <4 x float>* %75, align 4, !tbaa !301
  %76 = add nuw nsw i64 %4, 36
  %77 = getelementptr inbounds float, float* %2, i64 %76
  %78 = getelementptr inbounds float, float* %3, i64 %76
  %79 = bitcast float* %77 to <4 x float>*
  %80 = load <4 x float>, <4 x float>* %79, align 4, !tbaa !298
  %81 = fcmp ogt <4 x float> %80, zeroinitializer
  %82 = select <4 x i1> %81, <4 x float> %80, <4 x float> zeroinitializer
  %83 = bitcast float* %78 to <4 x float>*
  store <4 x float> %82, <4 x float>* %83, align 4, !tbaa !301
  %84 = add nuw nsw i64 %4, 40
  %85 = getelementptr inbounds float, float* %2, i64 %84
  %86 = getelementptr inbounds float, float* %3, i64 %84
  %87 = bitcast float* %85 to <4 x float>*
  %88 = load <4 x float>, <4 x float>* %87, align 4, !tbaa !298
  %89 = fcmp ogt <4 x float> %88, zeroinitializer
  %90 = select <4 x i1> %89, <4 x float> %88, <4 x float> zeroinitializer
  %91 = bitcast float* %86 to <4 x float>*
  store <4 x float> %90, <4 x float>* %91, align 4, !tbaa !301
  %92 = add nuw nsw i64 %4, 44
  %93 = getelementptr inbounds float, float* %2, i64 %92
  %94 = getelementptr inbounds float, float* %3, i64 %92
  %95 = bitcast float* %93 to <4 x float>*
  %96 = load <4 x float>, <4 x float>* %95, align 4, !tbaa !298
  %97 = fcmp ogt <4 x float> %96, zeroinitializer
  %98 = select <4 x i1> %97, <4 x float> %96, <4 x float> zeroinitializer
  %99 = bitcast float* %94 to <4 x float>*
  store <4 x float> %98, <4 x float>* %99, align 4, !tbaa !301
  %100 = add nuw nsw i64 %4, 48
  %101 = getelementptr inbounds float, float* %2, i64 %100
  %102 = getelementptr inbounds float, float* %3, i64 %100
  %103 = bitcast float* %101 to <4 x float>*
  %104 = load <4 x float>, <4 x float>* %103, align 4, !tbaa !298
  %105 = fcmp ogt <4 x float> %104, zeroinitializer
  %106 = select <4 x i1> %105, <4 x float> %104, <4 x float> zeroinitializer
  %107 = bitcast float* %102 to <4 x float>*
  store <4 x float> %106, <4 x float>* %107, align 4, !tbaa !301
  %108 = add nuw nsw i64 %4, 52
  %109 = getelementptr inbounds float, float* %2, i64 %108
  %110 = getelementptr inbounds float, float* %3, i64 %108
  %111 = bitcast float* %109 to <4 x float>*
  %112 = load <4 x float>, <4 x float>* %111, align 4, !tbaa !298
  %113 = fcmp ogt <4 x float> %112, zeroinitializer
  %114 = select <4 x i1> %113, <4 x float> %112, <4 x float> zeroinitializer
  %115 = bitcast float* %110 to <4 x float>*
  store <4 x float> %114, <4 x float>* %115, align 4, !tbaa !301
  %116 = add nuw nsw i64 %4, 56
  %117 = getelementptr inbounds float, float* %2, i64 %116
  %118 = getelementptr inbounds float, float* %3, i64 %116
  %119 = bitcast float* %117 to <4 x float>*
  %120 = load <4 x float>, <4 x float>* %119, align 4, !tbaa !298
  %121 = fcmp ogt <4 x float> %120, zeroinitializer
  %122 = select <4 x i1> %121, <4 x float> %120, <4 x float> zeroinitializer
  %123 = bitcast float* %118 to <4 x float>*
  store <4 x float> %122, <4 x float>* %123, align 4, !tbaa !301
  %124 = add nuw nsw i64 %4, 60
  %125 = getelementptr inbounds float, float* %2, i64 %124
  %126 = getelementptr inbounds float, float* %3, i64 %124
  %127 = bitcast float* %125 to <4 x float>*
  %128 = load <4 x float>, <4 x float>* %127, align 4, !tbaa !298
  %129 = fcmp ogt <4 x float> %128, zeroinitializer
  %130 = select <4 x i1> %129, <4 x float> %128, <4 x float> zeroinitializer
  %131 = bitcast float* %126 to <4 x float>*
  store <4 x float> %130, <4 x float>* %131, align 4, !tbaa !301
  %132 = add nuw nsw i64 %4, 64
  %133 = getelementptr inbounds float, float* %2, i64 %132
  %134 = getelementptr inbounds float, float* %3, i64 %132
  %135 = bitcast float* %133 to <4 x float>*
  %136 = load <4 x float>, <4 x float>* %135, align 4, !tbaa !298
  %137 = fcmp ogt <4 x float> %136, zeroinitializer
  %138 = select <4 x i1> %137, <4 x float> %136, <4 x float> zeroinitializer
  %139 = bitcast float* %134 to <4 x float>*
  store <4 x float> %138, <4 x float>* %139, align 4, !tbaa !301
  %140 = add nuw nsw i64 %4, 68
  %141 = getelementptr inbounds float, float* %2, i64 %140
  %142 = getelementptr inbounds float, float* %3, i64 %140
  %143 = bitcast float* %141 to <4 x float>*
  %144 = load <4 x float>, <4 x float>* %143, align 4, !tbaa !298
  %145 = fcmp ogt <4 x float> %144, zeroinitializer
  %146 = select <4 x i1> %145, <4 x float> %144, <4 x float> zeroinitializer
  %147 = bitcast float* %142 to <4 x float>*
  store <4 x float> %146, <4 x float>* %147, align 4, !tbaa !301
  %148 = add nuw nsw i64 %4, 72
  %149 = getelementptr inbounds float, float* %2, i64 %148
  %150 = getelementptr inbounds float, float* %3, i64 %148
  %151 = bitcast float* %149 to <4 x float>*
  %152 = load <4 x float>, <4 x float>* %151, align 4, !tbaa !298
  %153 = fcmp ogt <4 x float> %152, zeroinitializer
  %154 = select <4 x i1> %153, <4 x float> %152, <4 x float> zeroinitializer
  %155 = bitcast float* %150 to <4 x float>*
  store <4 x float> %154, <4 x float>* %155, align 4, !tbaa !301
  %156 = add nuw nsw i64 %4, 76
  %157 = getelementptr inbounds float, float* %2, i64 %156
  %158 = getelementptr inbounds float, float* %3, i64 %156
  %159 = bitcast float* %157 to <4 x float>*
  %160 = load <4 x float>, <4 x float>* %159, align 4, !tbaa !298
  %161 = fcmp ogt <4 x float> %160, zeroinitializer
  %162 = select <4 x i1> %161, <4 x float> %160, <4 x float> zeroinitializer
  %163 = bitcast float* %158 to <4 x float>*
  store <4 x float> %162, <4 x float>* %163, align 4, !tbaa !301
  %164 = add nuw nsw i64 %4, 80
  %165 = getelementptr inbounds float, float* %2, i64 %164
  %166 = getelementptr inbounds float, float* %3, i64 %164
  %167 = bitcast float* %165 to <4 x float>*
  %168 = load <4 x float>, <4 x float>* %167, align 4, !tbaa !298
  %169 = fcmp ogt <4 x float> %168, zeroinitializer
  %170 = select <4 x i1> %169, <4 x float> %168, <4 x float> zeroinitializer
  %171 = bitcast float* %166 to <4 x float>*
  store <4 x float> %170, <4 x float>* %171, align 4, !tbaa !301
  %172 = add nuw nsw i64 %4, 84
  %173 = getelementptr inbounds float, float* %2, i64 %172
  %174 = getelementptr inbounds float, float* %3, i64 %172
  %175 = bitcast float* %173 to <4 x float>*
  %176 = load <4 x float>, <4 x float>* %175, align 4, !tbaa !298
  %177 = fcmp ogt <4 x float> %176, zeroinitializer
  %178 = select <4 x i1> %177, <4 x float> %176, <4 x float> zeroinitializer
  %179 = bitcast float* %174 to <4 x float>*
  store <4 x float> %178, <4 x float>* %179, align 4, !tbaa !301
  %180 = add nuw nsw i64 %4, 88
  %181 = getelementptr inbounds float, float* %2, i64 %180
  %182 = getelementptr inbounds float, float* %3, i64 %180
  %183 = bitcast float* %181 to <4 x float>*
  %184 = load <4 x float>, <4 x float>* %183, align 4, !tbaa !298
  %185 = fcmp ogt <4 x float> %184, zeroinitializer
  %186 = select <4 x i1> %185, <4 x float> %184, <4 x float> zeroinitializer
  %187 = bitcast float* %182 to <4 x float>*
  store <4 x float> %186, <4 x float>* %187, align 4, !tbaa !301
  %188 = add nuw nsw i64 %4, 92
  %189 = getelementptr inbounds float, float* %2, i64 %188
  %190 = getelementptr inbounds float, float* %3, i64 %188
  %191 = bitcast float* %189 to <4 x float>*
  %192 = load <4 x float>, <4 x float>* %191, align 4, !tbaa !298
  %193 = fcmp ogt <4 x float> %192, zeroinitializer
  %194 = select <4 x i1> %193, <4 x float> %192, <4 x float> zeroinitializer
  %195 = bitcast float* %190 to <4 x float>*
  store <4 x float> %194, <4 x float>* %195, align 4, !tbaa !301
  %196 = add nuw nsw i64 %4, 96
  %197 = getelementptr inbounds float, float* %2, i64 %196
  %198 = getelementptr inbounds float, float* %3, i64 %196
  %199 = bitcast float* %197 to <4 x float>*
  %200 = load <4 x float>, <4 x float>* %199, align 4, !tbaa !298
  %201 = fcmp ogt <4 x float> %200, zeroinitializer
  %202 = select <4 x i1> %201, <4 x float> %200, <4 x float> zeroinitializer
  %203 = bitcast float* %198 to <4 x float>*
  store <4 x float> %202, <4 x float>* %203, align 4, !tbaa !301
  %204 = add nuw nsw i64 %4, 100
  %205 = getelementptr inbounds float, float* %2, i64 %204
  %206 = getelementptr inbounds float, float* %3, i64 %204
  %207 = bitcast float* %205 to <4 x float>*
  %208 = load <4 x float>, <4 x float>* %207, align 4, !tbaa !298
  %209 = fcmp ogt <4 x float> %208, zeroinitializer
  %210 = select <4 x i1> %209, <4 x float> %208, <4 x float> zeroinitializer
  %211 = bitcast float* %206 to <4 x float>*
  store <4 x float> %210, <4 x float>* %211, align 4, !tbaa !301
  %212 = add nuw nsw i64 %4, 104
  %213 = getelementptr inbounds float, float* %2, i64 %212
  %214 = getelementptr inbounds float, float* %3, i64 %212
  %215 = bitcast float* %213 to <4 x float>*
  %216 = load <4 x float>, <4 x float>* %215, align 4, !tbaa !298
  %217 = fcmp ogt <4 x float> %216, zeroinitializer
  %218 = select <4 x i1> %217, <4 x float> %216, <4 x float> zeroinitializer
  %219 = bitcast float* %214 to <4 x float>*
  store <4 x float> %218, <4 x float>* %219, align 4, !tbaa !301
  %220 = add nuw nsw i64 %4, 108
  %221 = getelementptr inbounds float, float* %2, i64 %220
  %222 = getelementptr inbounds float, float* %3, i64 %220
  %223 = bitcast float* %221 to <4 x float>*
  %224 = load <4 x float>, <4 x float>* %223, align 4, !tbaa !298
  %225 = fcmp ogt <4 x float> %224, zeroinitializer
  %226 = select <4 x i1> %225, <4 x float> %224, <4 x float> zeroinitializer
  %227 = bitcast float* %222 to <4 x float>*
  store <4 x float> %226, <4 x float>* %227, align 4, !tbaa !301
  %228 = add nuw nsw i64 %4, 112
  %229 = getelementptr inbounds float, float* %2, i64 %228
  %230 = getelementptr inbounds float, float* %3, i64 %228
  %231 = bitcast float* %229 to <4 x float>*
  %232 = load <4 x float>, <4 x float>* %231, align 4, !tbaa !298
  %233 = fcmp ogt <4 x float> %232, zeroinitializer
  %234 = select <4 x i1> %233, <4 x float> %232, <4 x float> zeroinitializer
  %235 = bitcast float* %230 to <4 x float>*
  store <4 x float> %234, <4 x float>* %235, align 4, !tbaa !301
  %236 = add nuw nsw i64 %4, 116
  %237 = getelementptr inbounds float, float* %2, i64 %236
  %238 = getelementptr inbounds float, float* %3, i64 %236
  %239 = bitcast float* %237 to <4 x float>*
  %240 = load <4 x float>, <4 x float>* %239, align 4, !tbaa !298
  %241 = fcmp ogt <4 x float> %240, zeroinitializer
  %242 = select <4 x i1> %241, <4 x float> %240, <4 x float> zeroinitializer
  %243 = bitcast float* %238 to <4 x float>*
  store <4 x float> %242, <4 x float>* %243, align 4, !tbaa !301
  %244 = add nuw nsw i64 %4, 120
  %245 = getelementptr inbounds float, float* %2, i64 %244
  %246 = getelementptr inbounds float, float* %3, i64 %244
  %247 = bitcast float* %245 to <4 x float>*
  %248 = load <4 x float>, <4 x float>* %247, align 4, !tbaa !298
  %249 = fcmp ogt <4 x float> %248, zeroinitializer
  %250 = select <4 x i1> %249, <4 x float> %248, <4 x float> zeroinitializer
  %251 = bitcast float* %246 to <4 x float>*
  store <4 x float> %250, <4 x float>* %251, align 4, !tbaa !301
  %252 = add nuw nsw i64 %4, 124
  %253 = getelementptr inbounds float, float* %2, i64 %252
  %254 = getelementptr inbounds float, float* %3, i64 %252
  %255 = bitcast float* %253 to <4 x float>*
  %256 = load <4 x float>, <4 x float>* %255, align 4, !tbaa !298
  %257 = fcmp ogt <4 x float> %256, zeroinitializer
  %258 = select <4 x i1> %257, <4 x float> %256, <4 x float> zeroinitializer
  %259 = bitcast float* %254 to <4 x float>*
  store <4 x float> %258, <4 x float>* %259, align 4, !tbaa !301
  %260 = add nuw nsw i64 %4, 128
  %261 = getelementptr inbounds float, float* %2, i64 %260
  %262 = getelementptr inbounds float, float* %3, i64 %260
  %263 = bitcast float* %261 to <4 x float>*
  %264 = load <4 x float>, <4 x float>* %263, align 4, !tbaa !298
  %265 = fcmp ogt <4 x float> %264, zeroinitializer
  %266 = select <4 x i1> %265, <4 x float> %264, <4 x float> zeroinitializer
  %267 = bitcast float* %262 to <4 x float>*
  store <4 x float> %266, <4 x float>* %267, align 4, !tbaa !301
  %268 = add nuw nsw i64 %4, 132
  %269 = getelementptr inbounds float, float* %2, i64 %268
  %270 = getelementptr inbounds float, float* %3, i64 %268
  %271 = bitcast float* %269 to <4 x float>*
  %272 = load <4 x float>, <4 x float>* %271, align 4, !tbaa !298
  %273 = fcmp ogt <4 x float> %272, zeroinitializer
  %274 = select <4 x i1> %273, <4 x float> %272, <4 x float> zeroinitializer
  %275 = bitcast float* %270 to <4 x float>*
  store <4 x float> %274, <4 x float>* %275, align 4, !tbaa !301
  %276 = add nuw nsw i64 %4, 136
  %277 = getelementptr inbounds float, float* %2, i64 %276
  %278 = getelementptr inbounds float, float* %3, i64 %276
  %279 = bitcast float* %277 to <4 x float>*
  %280 = load <4 x float>, <4 x float>* %279, align 4, !tbaa !298
  %281 = fcmp ogt <4 x float> %280, zeroinitializer
  %282 = select <4 x i1> %281, <4 x float> %280, <4 x float> zeroinitializer
  %283 = bitcast float* %278 to <4 x float>*
  store <4 x float> %282, <4 x float>* %283, align 4, !tbaa !301
  %284 = add nuw nsw i64 %4, 140
  %285 = getelementptr inbounds float, float* %2, i64 %284
  %286 = getelementptr inbounds float, float* %3, i64 %284
  %287 = bitcast float* %285 to <4 x float>*
  %288 = load <4 x float>, <4 x float>* %287, align 4, !tbaa !298
  %289 = fcmp ogt <4 x float> %288, zeroinitializer
  %290 = select <4 x i1> %289, <4 x float> %288, <4 x float> zeroinitializer
  %291 = bitcast float* %286 to <4 x float>*
  store <4 x float> %290, <4 x float>* %291, align 4, !tbaa !301
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 512
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !28

for_end:                                          ; preds = %for_begin1.preheader
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
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ]
  %4 = getelementptr inbounds float, float* %2, i64 %index
  %5 = bitcast float* %4 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %5, align 4, !tbaa !304
  %6 = getelementptr inbounds float, float* %4, i64 4
  %7 = bitcast float* %6 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %7, align 4, !tbaa !304
  %8 = fcmp ogt <4 x float> %wide.load, zeroinitializer
  %9 = fcmp ogt <4 x float> %wide.load2, zeroinitializer
  %10 = select <4 x i1> %8, <4 x float> %wide.load, <4 x float> zeroinitializer
  %11 = select <4 x i1> %9, <4 x float> %wide.load2, <4 x float> zeroinitializer
  %12 = getelementptr inbounds float, float* %3, i64 %index
  %13 = bitcast float* %12 to <4 x float>*
  store <4 x float> %10, <4 x float>* %13, align 4, !tbaa !307
  %14 = getelementptr inbounds float, float* %12, i64 4
  %15 = bitcast float* %14 to <4 x float>*
  store <4 x float> %11, <4 x float>* %15, align 4, !tbaa !307
  %index.next = add i64 %index, 8
  %16 = icmp eq i64 %index.next, 4096
  br i1 %16, label %for_end, label %vector.body, !llvm.loop !310

for_end:                                          ; preds = %vector.body
  ret void
}

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
  %3 = alloca [12 x <8 x float>], align 16
  %4 = alloca [147456 x <8 x float>], align 16
  %5 = alloca [50176 x float], align 16
  %6 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar = phi i64 [ 0, %entry ], [ %indvar.next, %for_end3 ]
  %7 = mul nuw nsw i64 %indvar, 112
  %8 = trunc i64 %indvar to i32
  %9 = urem i32 %8, 14
  %10 = udiv i32 %8, 14
  %.off = add nsw i32 %9, -1
  %11 = icmp ult i32 %.off, 12
  br i1 %11, label %for_begin4.preheader.us.preheader, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep142 = getelementptr [50176 x float], [50176 x float]* %5, i64 0, i64 %7
  %scevgep142143 = bitcast float* %scevgep142 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep142143, i8 0, i64 448, i1 false)
  br label %for_end3

for_begin4.preheader.us.preheader:                ; preds = %for_begin1.preheader
  %12 = mul nsw i32 %10, 1152
  %13 = add nsw i32 %12, -13
  %14 = mul nuw nsw i32 %9, 12
  %15 = add i32 %13, %14
  %16 = sext i32 %15 to i64
  %17 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %7
  store float 0.000000e+00, float* %17, align 16, !tbaa !311
  %18 = or i64 %7, 1
  %19 = add nsw i64 %16, 1
  %20 = getelementptr inbounds float, float* %6, i64 %19
  %21 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %18
  %22 = bitcast float* %20 to <4 x i32>*
  %23 = load <4 x i32>, <4 x i32>* %22, align 4, !tbaa !314
  %24 = bitcast float* %21 to <4 x i32>*
  store <4 x i32> %23, <4 x i32>* %24, align 4, !tbaa !311
  %25 = or i64 %7, 5
  %26 = add nsw i64 %16, 5
  %27 = getelementptr inbounds float, float* %6, i64 %26
  %28 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %25
  %29 = bitcast float* %27 to <4 x i32>*
  %30 = load <4 x i32>, <4 x i32>* %29, align 4, !tbaa !314
  %31 = bitcast float* %28 to <4 x i32>*
  store <4 x i32> %30, <4 x i32>* %31, align 4, !tbaa !311
  %32 = or i64 %7, 9
  %33 = add nsw i64 %16, 9
  %34 = getelementptr inbounds float, float* %6, i64 %33
  %35 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %32
  %36 = bitcast float* %34 to <4 x i32>*
  %37 = load <4 x i32>, <4 x i32>* %36, align 4, !tbaa !314
  %38 = bitcast float* %35 to <4 x i32>*
  store <4 x i32> %37, <4 x i32>* %38, align 4, !tbaa !311
  %39 = or i64 %7, 13
  %40 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %39
  store float 0.000000e+00, float* %40, align 4, !tbaa !311
  %41 = or i64 %7, 14
  %42 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %41
  store float 0.000000e+00, float* %42, align 8, !tbaa !311
  %43 = or i64 %7, 15
  %44 = add nsw i64 %16, 145
  %45 = getelementptr inbounds float, float* %6, i64 %44
  %46 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %43
  %47 = bitcast float* %45 to <4 x i32>*
  %48 = load <4 x i32>, <4 x i32>* %47, align 4, !tbaa !314
  %49 = bitcast float* %46 to <4 x i32>*
  store <4 x i32> %48, <4 x i32>* %49, align 4, !tbaa !311
  %50 = add nuw nsw i64 %41, 5
  %51 = add nsw i64 %16, 149
  %52 = getelementptr inbounds float, float* %6, i64 %51
  %53 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %50
  %54 = bitcast float* %52 to <4 x i32>*
  %55 = load <4 x i32>, <4 x i32>* %54, align 4, !tbaa !314
  %56 = bitcast float* %53 to <4 x i32>*
  store <4 x i32> %55, <4 x i32>* %56, align 4, !tbaa !311
  %57 = add nuw nsw i64 %41, 9
  %58 = add nsw i64 %16, 153
  %59 = getelementptr inbounds float, float* %6, i64 %58
  %60 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %57
  %61 = bitcast float* %59 to <4 x i32>*
  %62 = load <4 x i32>, <4 x i32>* %61, align 4, !tbaa !314
  %63 = bitcast float* %60 to <4 x i32>*
  store <4 x i32> %62, <4 x i32>* %63, align 4, !tbaa !311
  %64 = add nuw nsw i64 %41, 13
  %65 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %64
  store float 0.000000e+00, float* %65, align 4, !tbaa !311
  %66 = add nuw nsw i64 %7, 28
  %67 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %66
  store float 0.000000e+00, float* %67, align 16, !tbaa !311
  %68 = or i64 %66, 1
  %69 = add nsw i64 %16, 289
  %70 = getelementptr inbounds float, float* %6, i64 %69
  %71 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %68
  %72 = bitcast float* %70 to <4 x i32>*
  %73 = load <4 x i32>, <4 x i32>* %72, align 4, !tbaa !314
  %74 = bitcast float* %71 to <4 x i32>*
  store <4 x i32> %73, <4 x i32>* %74, align 4, !tbaa !311
  %75 = add nuw nsw i64 %7, 33
  %76 = add nsw i64 %16, 293
  %77 = getelementptr inbounds float, float* %6, i64 %76
  %78 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %75
  %79 = bitcast float* %77 to <4 x i32>*
  %80 = load <4 x i32>, <4 x i32>* %79, align 4, !tbaa !314
  %81 = bitcast float* %78 to <4 x i32>*
  store <4 x i32> %80, <4 x i32>* %81, align 4, !tbaa !311
  %82 = add nuw nsw i64 %7, 37
  %83 = add nsw i64 %16, 297
  %84 = getelementptr inbounds float, float* %6, i64 %83
  %85 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %82
  %86 = bitcast float* %84 to <4 x i32>*
  %87 = load <4 x i32>, <4 x i32>* %86, align 4, !tbaa !314
  %88 = bitcast float* %85 to <4 x i32>*
  store <4 x i32> %87, <4 x i32>* %88, align 4, !tbaa !311
  %89 = add nuw nsw i64 %7, 41
  %90 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %89
  store float 0.000000e+00, float* %90, align 4, !tbaa !311
  %91 = add nuw nsw i64 %7, 42
  %92 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %91
  store float 0.000000e+00, float* %92, align 8, !tbaa !311
  %93 = or i64 %91, 1
  %94 = add nsw i64 %16, 433
  %95 = getelementptr inbounds float, float* %6, i64 %94
  %96 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %93
  %97 = bitcast float* %95 to <4 x i32>*
  %98 = load <4 x i32>, <4 x i32>* %97, align 4, !tbaa !314
  %99 = bitcast float* %96 to <4 x i32>*
  store <4 x i32> %98, <4 x i32>* %99, align 4, !tbaa !311
  %100 = add nuw nsw i64 %7, 47
  %101 = add nsw i64 %16, 437
  %102 = getelementptr inbounds float, float* %6, i64 %101
  %103 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %100
  %104 = bitcast float* %102 to <4 x i32>*
  %105 = load <4 x i32>, <4 x i32>* %104, align 4, !tbaa !314
  %106 = bitcast float* %103 to <4 x i32>*
  store <4 x i32> %105, <4 x i32>* %106, align 4, !tbaa !311
  %107 = add nuw nsw i64 %7, 51
  %108 = add nsw i64 %16, 441
  %109 = getelementptr inbounds float, float* %6, i64 %108
  %110 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %107
  %111 = bitcast float* %109 to <4 x i32>*
  %112 = load <4 x i32>, <4 x i32>* %111, align 4, !tbaa !314
  %113 = bitcast float* %110 to <4 x i32>*
  store <4 x i32> %112, <4 x i32>* %113, align 4, !tbaa !311
  %114 = add nuw nsw i64 %7, 55
  %115 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %114
  store float 0.000000e+00, float* %115, align 4, !tbaa !311
  %116 = add nuw nsw i64 %7, 56
  %117 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %116
  store float 0.000000e+00, float* %117, align 16, !tbaa !311
  %118 = or i64 %116, 1
  %119 = add nsw i64 %16, 577
  %120 = getelementptr inbounds float, float* %6, i64 %119
  %121 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %118
  %122 = bitcast float* %120 to <4 x i32>*
  %123 = load <4 x i32>, <4 x i32>* %122, align 4, !tbaa !314
  %124 = bitcast float* %121 to <4 x i32>*
  store <4 x i32> %123, <4 x i32>* %124, align 4, !tbaa !311
  %125 = add nuw nsw i64 %7, 61
  %126 = add nsw i64 %16, 581
  %127 = getelementptr inbounds float, float* %6, i64 %126
  %128 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %125
  %129 = bitcast float* %127 to <4 x i32>*
  %130 = load <4 x i32>, <4 x i32>* %129, align 4, !tbaa !314
  %131 = bitcast float* %128 to <4 x i32>*
  store <4 x i32> %130, <4 x i32>* %131, align 4, !tbaa !311
  %132 = add nuw nsw i64 %7, 65
  %133 = add nsw i64 %16, 585
  %134 = getelementptr inbounds float, float* %6, i64 %133
  %135 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %132
  %136 = bitcast float* %134 to <4 x i32>*
  %137 = load <4 x i32>, <4 x i32>* %136, align 4, !tbaa !314
  %138 = bitcast float* %135 to <4 x i32>*
  store <4 x i32> %137, <4 x i32>* %138, align 4, !tbaa !311
  %139 = add nuw nsw i64 %7, 69
  %140 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %139
  store float 0.000000e+00, float* %140, align 4, !tbaa !311
  %141 = add nuw nsw i64 %7, 70
  %142 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %141
  store float 0.000000e+00, float* %142, align 8, !tbaa !311
  %143 = or i64 %141, 1
  %144 = add nsw i64 %16, 721
  %145 = getelementptr inbounds float, float* %6, i64 %144
  %146 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %143
  %147 = bitcast float* %145 to <4 x i32>*
  %148 = load <4 x i32>, <4 x i32>* %147, align 4, !tbaa !314
  %149 = bitcast float* %146 to <4 x i32>*
  store <4 x i32> %148, <4 x i32>* %149, align 4, !tbaa !311
  %150 = add nuw nsw i64 %7, 75
  %151 = add nsw i64 %16, 725
  %152 = getelementptr inbounds float, float* %6, i64 %151
  %153 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %150
  %154 = bitcast float* %152 to <4 x i32>*
  %155 = load <4 x i32>, <4 x i32>* %154, align 4, !tbaa !314
  %156 = bitcast float* %153 to <4 x i32>*
  store <4 x i32> %155, <4 x i32>* %156, align 4, !tbaa !311
  %157 = add nuw nsw i64 %7, 79
  %158 = add nsw i64 %16, 729
  %159 = getelementptr inbounds float, float* %6, i64 %158
  %160 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %157
  %161 = bitcast float* %159 to <4 x i32>*
  %162 = load <4 x i32>, <4 x i32>* %161, align 4, !tbaa !314
  %163 = bitcast float* %160 to <4 x i32>*
  store <4 x i32> %162, <4 x i32>* %163, align 4, !tbaa !311
  %164 = add nuw nsw i64 %7, 83
  %165 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %164
  store float 0.000000e+00, float* %165, align 4, !tbaa !311
  %166 = add nuw nsw i64 %7, 84
  %167 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %166
  store float 0.000000e+00, float* %167, align 16, !tbaa !311
  %168 = or i64 %166, 1
  %169 = add nsw i64 %16, 865
  %170 = getelementptr inbounds float, float* %6, i64 %169
  %171 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %168
  %172 = bitcast float* %170 to <4 x i32>*
  %173 = load <4 x i32>, <4 x i32>* %172, align 4, !tbaa !314
  %174 = bitcast float* %171 to <4 x i32>*
  store <4 x i32> %173, <4 x i32>* %174, align 4, !tbaa !311
  %175 = add nuw nsw i64 %7, 89
  %176 = add nsw i64 %16, 869
  %177 = getelementptr inbounds float, float* %6, i64 %176
  %178 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %175
  %179 = bitcast float* %177 to <4 x i32>*
  %180 = load <4 x i32>, <4 x i32>* %179, align 4, !tbaa !314
  %181 = bitcast float* %178 to <4 x i32>*
  store <4 x i32> %180, <4 x i32>* %181, align 4, !tbaa !311
  %182 = add nuw nsw i64 %7, 93
  %183 = add nsw i64 %16, 873
  %184 = getelementptr inbounds float, float* %6, i64 %183
  %185 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %182
  %186 = bitcast float* %184 to <4 x i32>*
  %187 = load <4 x i32>, <4 x i32>* %186, align 4, !tbaa !314
  %188 = bitcast float* %185 to <4 x i32>*
  store <4 x i32> %187, <4 x i32>* %188, align 4, !tbaa !311
  %189 = add nuw nsw i64 %7, 97
  %190 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %189
  store float 0.000000e+00, float* %190, align 4, !tbaa !311
  %191 = add nuw nsw i64 %7, 98
  %192 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %191
  store float 0.000000e+00, float* %192, align 8, !tbaa !311
  %193 = or i64 %191, 1
  %194 = add nsw i64 %16, 1009
  %195 = getelementptr inbounds float, float* %6, i64 %194
  %196 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %193
  %197 = bitcast float* %195 to <4 x i32>*
  %198 = load <4 x i32>, <4 x i32>* %197, align 4, !tbaa !314
  %199 = bitcast float* %196 to <4 x i32>*
  store <4 x i32> %198, <4 x i32>* %199, align 4, !tbaa !311
  %200 = add nuw nsw i64 %7, 103
  %201 = add nsw i64 %16, 1013
  %202 = getelementptr inbounds float, float* %6, i64 %201
  %203 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %200
  %204 = bitcast float* %202 to <4 x i32>*
  %205 = load <4 x i32>, <4 x i32>* %204, align 4, !tbaa !314
  %206 = bitcast float* %203 to <4 x i32>*
  store <4 x i32> %205, <4 x i32>* %206, align 4, !tbaa !311
  %207 = add nuw nsw i64 %7, 107
  %208 = add nsw i64 %16, 1017
  %209 = getelementptr inbounds float, float* %6, i64 %208
  %210 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %207
  %211 = bitcast float* %209 to <4 x i32>*
  %212 = load <4 x i32>, <4 x i32>* %211, align 4, !tbaa !314
  %213 = bitcast float* %210 to <4 x i32>*
  store <4 x i32> %212, <4 x i32>* %213, align 4, !tbaa !311
  %214 = add nuw nsw i64 %7, 111
  %215 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %214
  store float 0.000000e+00, float* %215, align 4, !tbaa !311
  br label %for_end3

for_begin7.preheader:                             ; preds = %for_end3
  %.sub = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0
  %216 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_end3:                                         ; preds = %for_begin4.preheader.us.preheader, %for_begin4.preheader.preheader
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond150 = icmp eq i64 %indvar.next, 448
  br i1 %exitcond150, label %for_begin7.preheader, label %for_begin1.preheader, !prof !28

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %217 = phi i32 [ 0, %for_begin7.preheader ], [ %255, %for_end12 ]
  %218 = urem i32 %217, 3
  %219 = mul nuw nsw i32 %218, 192
  %220 = udiv i32 %217, 3
  %221 = mul nsw i32 %220, 18432
  %222 = or i32 %219, %221
  %223 = mul nuw nsw i32 %218, 3
  %224 = or i32 %223, %221
  %225 = zext i32 %224 to i64
  %226 = zext i32 %222 to i64
  br label %for_begin13.preheader

for_begin19.preheader:                            ; preds = %for_end12
  %227 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 8
  %228 = bitcast float* %227 to <8 x float>*
  %229 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 16
  %230 = bitcast float* %229 to <8 x float>*
  %231 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 24
  %232 = bitcast float* %231 to <8 x float>*
  %233 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 32
  %234 = bitcast float* %233 to <8 x float>*
  %235 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 40
  %236 = bitcast float* %235 to <8 x float>*
  %237 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 48
  %238 = bitcast float* %237 to <8 x float>*
  %239 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 56
  %240 = bitcast float* %239 to <8 x float>*
  %241 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 64
  %242 = bitcast float* %241 to <8 x float>*
  %243 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 72
  %244 = bitcast float* %243 to <8 x float>*
  %245 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 80
  %246 = bitcast float* %245 to <8 x float>*
  %247 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 88
  %248 = bitcast float* %247 to <8 x float>*
  %249 = bitcast i8* %2 to float*
  %250 = bitcast [12 x <8 x float>]* %3 to i8*
  br label %for_body20

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvars.iv131 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next132, %for_end15 ]
  %251 = mul nuw nsw i64 %indvars.iv131, 576
  %252 = add nuw nsw i64 %251, %226
  %253 = mul nuw nsw i64 %indvars.iv131, 72
  %254 = add nuw nsw i64 %253, %225
  br label %for_begin16.preheader

for_end12:                                        ; preds = %for_end15
  %255 = add nuw nsw i32 %217, 1
  %exitcond134 = icmp eq i32 %255, 192
  br i1 %exitcond134, label %for_begin19.preheader, label %for_begin10.preheader, !prof !28

for_begin16.preheader:                            ; preds = %for_begin16.preheader, %for_begin13.preheader
  %indvars.iv128 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next129, %for_begin16.preheader ]
  %256 = shl i64 %indvars.iv128, 6
  %257 = add nuw nsw i64 %252, %256
  %258 = add nuw nsw i64 %254, %indvars.iv128
  %259 = getelementptr inbounds float, float* %216, i64 %258
  %260 = load float, float* %259, align 4, !tbaa !317
  %261 = insertelement <8 x float> undef, float %260, i32 0
  %262 = shl i64 %258, 32
  %sext = add i64 %262, 9895604649984
  %263 = ashr exact i64 %sext, 32
  %264 = getelementptr inbounds float, float* %216, i64 %263
  %265 = load float, float* %264, align 4, !tbaa !317
  %266 = insertelement <8 x float> %261, float %265, i32 1
  %267 = shl i64 %258, 32
  %sext151 = add i64 %267, 19791209299968
  %268 = ashr exact i64 %sext151, 32
  %269 = getelementptr inbounds float, float* %216, i64 %268
  %270 = load float, float* %269, align 4, !tbaa !317
  %271 = insertelement <8 x float> %266, float %270, i32 2
  %272 = shl i64 %258, 32
  %sext152 = add i64 %272, 29686813949952
  %273 = ashr exact i64 %sext152, 32
  %274 = getelementptr inbounds float, float* %216, i64 %273
  %275 = load float, float* %274, align 4, !tbaa !317
  %276 = insertelement <8 x float> %271, float %275, i32 3
  %277 = shl i64 %258, 32
  %sext153 = add i64 %277, 39582418599936
  %278 = ashr exact i64 %sext153, 32
  %279 = getelementptr inbounds float, float* %216, i64 %278
  %280 = load float, float* %279, align 4, !tbaa !317
  %281 = insertelement <8 x float> %276, float %280, i32 4
  %282 = shl i64 %258, 32
  %sext154 = add i64 %282, 49478023249920
  %283 = ashr exact i64 %sext154, 32
  %284 = getelementptr inbounds float, float* %216, i64 %283
  %285 = load float, float* %284, align 4, !tbaa !317
  %286 = insertelement <8 x float> %281, float %285, i32 5
  %287 = shl i64 %258, 32
  %sext155 = add i64 %287, 59373627899904
  %288 = ashr exact i64 %sext155, 32
  %289 = getelementptr inbounds float, float* %216, i64 %288
  %290 = load float, float* %289, align 4, !tbaa !317
  %291 = insertelement <8 x float> %286, float %290, i32 6
  %292 = shl i64 %258, 32
  %sext156 = add i64 %292, 69269232549888
  %293 = ashr exact i64 %sext156, 32
  %294 = getelementptr inbounds float, float* %216, i64 %293
  %295 = load float, float* %294, align 4, !tbaa !317
  %296 = insertelement <8 x float> %291, float %295, i32 7
  %297 = getelementptr inbounds [147456 x <8 x float>], [147456 x <8 x float>]* %4, i64 0, i64 0, i64 %257
  %298 = bitcast float* %297 to <8 x float>*
  store <8 x float> %296, <8 x float>* %298, align 16, !tbaa !320
  %299 = or i64 %257, 8
  %300 = add nuw nsw i64 %258, 9
  %301 = getelementptr inbounds float, float* %216, i64 %300
  %302 = load float, float* %301, align 4, !tbaa !317
  %303 = insertelement <8 x float> undef, float %302, i32 0
  %304 = shl i64 %258, 32
  %sext157 = add i64 %304, 9934259355648
  %305 = ashr exact i64 %sext157, 32
  %306 = getelementptr inbounds float, float* %216, i64 %305
  %307 = load float, float* %306, align 4, !tbaa !317
  %308 = insertelement <8 x float> %303, float %307, i32 1
  %309 = shl i64 %258, 32
  %sext158 = add i64 %309, 19829864005632
  %310 = ashr exact i64 %sext158, 32
  %311 = getelementptr inbounds float, float* %216, i64 %310
  %312 = load float, float* %311, align 4, !tbaa !317
  %313 = insertelement <8 x float> %308, float %312, i32 2
  %314 = shl i64 %258, 32
  %sext159 = add i64 %314, 29725468655616
  %315 = ashr exact i64 %sext159, 32
  %316 = getelementptr inbounds float, float* %216, i64 %315
  %317 = load float, float* %316, align 4, !tbaa !317
  %318 = insertelement <8 x float> %313, float %317, i32 3
  %319 = shl i64 %258, 32
  %sext160 = add i64 %319, 39621073305600
  %320 = ashr exact i64 %sext160, 32
  %321 = getelementptr inbounds float, float* %216, i64 %320
  %322 = load float, float* %321, align 4, !tbaa !317
  %323 = insertelement <8 x float> %318, float %322, i32 4
  %324 = shl i64 %258, 32
  %sext161 = add i64 %324, 49516677955584
  %325 = ashr exact i64 %sext161, 32
  %326 = getelementptr inbounds float, float* %216, i64 %325
  %327 = load float, float* %326, align 4, !tbaa !317
  %328 = insertelement <8 x float> %323, float %327, i32 5
  %329 = shl i64 %258, 32
  %sext162 = add i64 %329, 59412282605568
  %330 = ashr exact i64 %sext162, 32
  %331 = getelementptr inbounds float, float* %216, i64 %330
  %332 = load float, float* %331, align 4, !tbaa !317
  %333 = insertelement <8 x float> %328, float %332, i32 6
  %334 = shl i64 %258, 32
  %sext163 = add i64 %334, 69307887255552
  %335 = ashr exact i64 %sext163, 32
  %336 = getelementptr inbounds float, float* %216, i64 %335
  %337 = load float, float* %336, align 4, !tbaa !317
  %338 = insertelement <8 x float> %333, float %337, i32 7
  %339 = getelementptr inbounds [147456 x <8 x float>], [147456 x <8 x float>]* %4, i64 0, i64 0, i64 %299
  %340 = bitcast float* %339 to <8 x float>*
  store <8 x float> %338, <8 x float>* %340, align 16, !tbaa !320
  %341 = or i64 %257, 16
  %342 = add nuw nsw i64 %258, 18
  %343 = getelementptr inbounds float, float* %216, i64 %342
  %344 = load float, float* %343, align 4, !tbaa !317
  %345 = insertelement <8 x float> undef, float %344, i32 0
  %346 = shl i64 %258, 32
  %sext164 = add i64 %346, 9972914061312
  %347 = ashr exact i64 %sext164, 32
  %348 = getelementptr inbounds float, float* %216, i64 %347
  %349 = load float, float* %348, align 4, !tbaa !317
  %350 = insertelement <8 x float> %345, float %349, i32 1
  %351 = shl i64 %258, 32
  %sext165 = add i64 %351, 19868518711296
  %352 = ashr exact i64 %sext165, 32
  %353 = getelementptr inbounds float, float* %216, i64 %352
  %354 = load float, float* %353, align 4, !tbaa !317
  %355 = insertelement <8 x float> %350, float %354, i32 2
  %356 = shl i64 %258, 32
  %sext166 = add i64 %356, 29764123361280
  %357 = ashr exact i64 %sext166, 32
  %358 = getelementptr inbounds float, float* %216, i64 %357
  %359 = load float, float* %358, align 4, !tbaa !317
  %360 = insertelement <8 x float> %355, float %359, i32 3
  %361 = shl i64 %258, 32
  %sext167 = add i64 %361, 39659728011264
  %362 = ashr exact i64 %sext167, 32
  %363 = getelementptr inbounds float, float* %216, i64 %362
  %364 = load float, float* %363, align 4, !tbaa !317
  %365 = insertelement <8 x float> %360, float %364, i32 4
  %366 = shl i64 %258, 32
  %sext168 = add i64 %366, 49555332661248
  %367 = ashr exact i64 %sext168, 32
  %368 = getelementptr inbounds float, float* %216, i64 %367
  %369 = load float, float* %368, align 4, !tbaa !317
  %370 = insertelement <8 x float> %365, float %369, i32 5
  %371 = shl i64 %258, 32
  %sext169 = add i64 %371, 59450937311232
  %372 = ashr exact i64 %sext169, 32
  %373 = getelementptr inbounds float, float* %216, i64 %372
  %374 = load float, float* %373, align 4, !tbaa !317
  %375 = insertelement <8 x float> %370, float %374, i32 6
  %376 = shl i64 %258, 32
  %sext170 = add i64 %376, 69346541961216
  %377 = ashr exact i64 %sext170, 32
  %378 = getelementptr inbounds float, float* %216, i64 %377
  %379 = load float, float* %378, align 4, !tbaa !317
  %380 = insertelement <8 x float> %375, float %379, i32 7
  %381 = getelementptr inbounds [147456 x <8 x float>], [147456 x <8 x float>]* %4, i64 0, i64 0, i64 %341
  %382 = bitcast float* %381 to <8 x float>*
  store <8 x float> %380, <8 x float>* %382, align 16, !tbaa !320
  %383 = or i64 %257, 24
  %384 = add nuw nsw i64 %258, 27
  %385 = getelementptr inbounds float, float* %216, i64 %384
  %386 = load float, float* %385, align 4, !tbaa !317
  %387 = insertelement <8 x float> undef, float %386, i32 0
  %388 = shl i64 %258, 32
  %sext171 = add i64 %388, 10011568766976
  %389 = ashr exact i64 %sext171, 32
  %390 = getelementptr inbounds float, float* %216, i64 %389
  %391 = load float, float* %390, align 4, !tbaa !317
  %392 = insertelement <8 x float> %387, float %391, i32 1
  %393 = shl i64 %258, 32
  %sext172 = add i64 %393, 19907173416960
  %394 = ashr exact i64 %sext172, 32
  %395 = getelementptr inbounds float, float* %216, i64 %394
  %396 = load float, float* %395, align 4, !tbaa !317
  %397 = insertelement <8 x float> %392, float %396, i32 2
  %398 = shl i64 %258, 32
  %sext173 = add i64 %398, 29802778066944
  %399 = ashr exact i64 %sext173, 32
  %400 = getelementptr inbounds float, float* %216, i64 %399
  %401 = load float, float* %400, align 4, !tbaa !317
  %402 = insertelement <8 x float> %397, float %401, i32 3
  %403 = shl i64 %258, 32
  %sext174 = add i64 %403, 39698382716928
  %404 = ashr exact i64 %sext174, 32
  %405 = getelementptr inbounds float, float* %216, i64 %404
  %406 = load float, float* %405, align 4, !tbaa !317
  %407 = insertelement <8 x float> %402, float %406, i32 4
  %408 = shl i64 %258, 32
  %sext175 = add i64 %408, 49593987366912
  %409 = ashr exact i64 %sext175, 32
  %410 = getelementptr inbounds float, float* %216, i64 %409
  %411 = load float, float* %410, align 4, !tbaa !317
  %412 = insertelement <8 x float> %407, float %411, i32 5
  %413 = shl i64 %258, 32
  %sext176 = add i64 %413, 59489592016896
  %414 = ashr exact i64 %sext176, 32
  %415 = getelementptr inbounds float, float* %216, i64 %414
  %416 = load float, float* %415, align 4, !tbaa !317
  %417 = insertelement <8 x float> %412, float %416, i32 6
  %418 = shl i64 %258, 32
  %sext177 = add i64 %418, 69385196666880
  %419 = ashr exact i64 %sext177, 32
  %420 = getelementptr inbounds float, float* %216, i64 %419
  %421 = load float, float* %420, align 4, !tbaa !317
  %422 = insertelement <8 x float> %417, float %421, i32 7
  %423 = getelementptr inbounds [147456 x <8 x float>], [147456 x <8 x float>]* %4, i64 0, i64 0, i64 %383
  %424 = bitcast float* %423 to <8 x float>*
  store <8 x float> %422, <8 x float>* %424, align 16, !tbaa !320
  %425 = or i64 %257, 32
  %426 = add nuw nsw i64 %258, 36
  %427 = getelementptr inbounds float, float* %216, i64 %426
  %428 = load float, float* %427, align 4, !tbaa !317
  %429 = insertelement <8 x float> undef, float %428, i32 0
  %430 = shl i64 %258, 32
  %sext178 = add i64 %430, 10050223472640
  %431 = ashr exact i64 %sext178, 32
  %432 = getelementptr inbounds float, float* %216, i64 %431
  %433 = load float, float* %432, align 4, !tbaa !317
  %434 = insertelement <8 x float> %429, float %433, i32 1
  %435 = shl i64 %258, 32
  %sext179 = add i64 %435, 19945828122624
  %436 = ashr exact i64 %sext179, 32
  %437 = getelementptr inbounds float, float* %216, i64 %436
  %438 = load float, float* %437, align 4, !tbaa !317
  %439 = insertelement <8 x float> %434, float %438, i32 2
  %440 = shl i64 %258, 32
  %sext180 = add i64 %440, 29841432772608
  %441 = ashr exact i64 %sext180, 32
  %442 = getelementptr inbounds float, float* %216, i64 %441
  %443 = load float, float* %442, align 4, !tbaa !317
  %444 = insertelement <8 x float> %439, float %443, i32 3
  %445 = shl i64 %258, 32
  %sext181 = add i64 %445, 39737037422592
  %446 = ashr exact i64 %sext181, 32
  %447 = getelementptr inbounds float, float* %216, i64 %446
  %448 = load float, float* %447, align 4, !tbaa !317
  %449 = insertelement <8 x float> %444, float %448, i32 4
  %450 = shl i64 %258, 32
  %sext182 = add i64 %450, 49632642072576
  %451 = ashr exact i64 %sext182, 32
  %452 = getelementptr inbounds float, float* %216, i64 %451
  %453 = load float, float* %452, align 4, !tbaa !317
  %454 = insertelement <8 x float> %449, float %453, i32 5
  %455 = shl i64 %258, 32
  %sext183 = add i64 %455, 59528246722560
  %456 = ashr exact i64 %sext183, 32
  %457 = getelementptr inbounds float, float* %216, i64 %456
  %458 = load float, float* %457, align 4, !tbaa !317
  %459 = insertelement <8 x float> %454, float %458, i32 6
  %460 = shl i64 %258, 32
  %sext184 = add i64 %460, 69423851372544
  %461 = ashr exact i64 %sext184, 32
  %462 = getelementptr inbounds float, float* %216, i64 %461
  %463 = load float, float* %462, align 4, !tbaa !317
  %464 = insertelement <8 x float> %459, float %463, i32 7
  %465 = getelementptr inbounds [147456 x <8 x float>], [147456 x <8 x float>]* %4, i64 0, i64 0, i64 %425
  %466 = bitcast float* %465 to <8 x float>*
  store <8 x float> %464, <8 x float>* %466, align 16, !tbaa !320
  %467 = or i64 %257, 40
  %468 = add nuw nsw i64 %258, 45
  %469 = getelementptr inbounds float, float* %216, i64 %468
  %470 = load float, float* %469, align 4, !tbaa !317
  %471 = insertelement <8 x float> undef, float %470, i32 0
  %472 = shl i64 %258, 32
  %sext185 = add i64 %472, 10088878178304
  %473 = ashr exact i64 %sext185, 32
  %474 = getelementptr inbounds float, float* %216, i64 %473
  %475 = load float, float* %474, align 4, !tbaa !317
  %476 = insertelement <8 x float> %471, float %475, i32 1
  %477 = shl i64 %258, 32
  %sext186 = add i64 %477, 19984482828288
  %478 = ashr exact i64 %sext186, 32
  %479 = getelementptr inbounds float, float* %216, i64 %478
  %480 = load float, float* %479, align 4, !tbaa !317
  %481 = insertelement <8 x float> %476, float %480, i32 2
  %482 = shl i64 %258, 32
  %sext187 = add i64 %482, 29880087478272
  %483 = ashr exact i64 %sext187, 32
  %484 = getelementptr inbounds float, float* %216, i64 %483
  %485 = load float, float* %484, align 4, !tbaa !317
  %486 = insertelement <8 x float> %481, float %485, i32 3
  %487 = shl i64 %258, 32
  %sext188 = add i64 %487, 39775692128256
  %488 = ashr exact i64 %sext188, 32
  %489 = getelementptr inbounds float, float* %216, i64 %488
  %490 = load float, float* %489, align 4, !tbaa !317
  %491 = insertelement <8 x float> %486, float %490, i32 4
  %492 = shl i64 %258, 32
  %sext189 = add i64 %492, 49671296778240
  %493 = ashr exact i64 %sext189, 32
  %494 = getelementptr inbounds float, float* %216, i64 %493
  %495 = load float, float* %494, align 4, !tbaa !317
  %496 = insertelement <8 x float> %491, float %495, i32 5
  %497 = shl i64 %258, 32
  %sext190 = add i64 %497, 59566901428224
  %498 = ashr exact i64 %sext190, 32
  %499 = getelementptr inbounds float, float* %216, i64 %498
  %500 = load float, float* %499, align 4, !tbaa !317
  %501 = insertelement <8 x float> %496, float %500, i32 6
  %502 = shl i64 %258, 32
  %sext191 = add i64 %502, 69462506078208
  %503 = ashr exact i64 %sext191, 32
  %504 = getelementptr inbounds float, float* %216, i64 %503
  %505 = load float, float* %504, align 4, !tbaa !317
  %506 = insertelement <8 x float> %501, float %505, i32 7
  %507 = getelementptr inbounds [147456 x <8 x float>], [147456 x <8 x float>]* %4, i64 0, i64 0, i64 %467
  %508 = bitcast float* %507 to <8 x float>*
  store <8 x float> %506, <8 x float>* %508, align 16, !tbaa !320
  %509 = or i64 %257, 48
  %510 = add nuw nsw i64 %258, 54
  %511 = getelementptr inbounds float, float* %216, i64 %510
  %512 = load float, float* %511, align 4, !tbaa !317
  %513 = insertelement <8 x float> undef, float %512, i32 0
  %514 = shl i64 %258, 32
  %sext192 = add i64 %514, 10127532883968
  %515 = ashr exact i64 %sext192, 32
  %516 = getelementptr inbounds float, float* %216, i64 %515
  %517 = load float, float* %516, align 4, !tbaa !317
  %518 = insertelement <8 x float> %513, float %517, i32 1
  %519 = shl i64 %258, 32
  %sext193 = add i64 %519, 20023137533952
  %520 = ashr exact i64 %sext193, 32
  %521 = getelementptr inbounds float, float* %216, i64 %520
  %522 = load float, float* %521, align 4, !tbaa !317
  %523 = insertelement <8 x float> %518, float %522, i32 2
  %524 = shl i64 %258, 32
  %sext194 = add i64 %524, 29918742183936
  %525 = ashr exact i64 %sext194, 32
  %526 = getelementptr inbounds float, float* %216, i64 %525
  %527 = load float, float* %526, align 4, !tbaa !317
  %528 = insertelement <8 x float> %523, float %527, i32 3
  %529 = shl i64 %258, 32
  %sext195 = add i64 %529, 39814346833920
  %530 = ashr exact i64 %sext195, 32
  %531 = getelementptr inbounds float, float* %216, i64 %530
  %532 = load float, float* %531, align 4, !tbaa !317
  %533 = insertelement <8 x float> %528, float %532, i32 4
  %534 = shl i64 %258, 32
  %sext196 = add i64 %534, 49709951483904
  %535 = ashr exact i64 %sext196, 32
  %536 = getelementptr inbounds float, float* %216, i64 %535
  %537 = load float, float* %536, align 4, !tbaa !317
  %538 = insertelement <8 x float> %533, float %537, i32 5
  %539 = shl i64 %258, 32
  %sext197 = add i64 %539, 59605556133888
  %540 = ashr exact i64 %sext197, 32
  %541 = getelementptr inbounds float, float* %216, i64 %540
  %542 = load float, float* %541, align 4, !tbaa !317
  %543 = insertelement <8 x float> %538, float %542, i32 6
  %544 = shl i64 %258, 32
  %sext198 = add i64 %544, 69501160783872
  %545 = ashr exact i64 %sext198, 32
  %546 = getelementptr inbounds float, float* %216, i64 %545
  %547 = load float, float* %546, align 4, !tbaa !317
  %548 = insertelement <8 x float> %543, float %547, i32 7
  %549 = getelementptr inbounds [147456 x <8 x float>], [147456 x <8 x float>]* %4, i64 0, i64 0, i64 %509
  %550 = bitcast float* %549 to <8 x float>*
  store <8 x float> %548, <8 x float>* %550, align 16, !tbaa !320
  %551 = or i64 %257, 56
  %552 = add nuw nsw i64 %258, 63
  %553 = getelementptr inbounds float, float* %216, i64 %552
  %554 = load float, float* %553, align 4, !tbaa !317
  %555 = insertelement <8 x float> undef, float %554, i32 0
  %556 = shl i64 %258, 32
  %sext199 = add i64 %556, 10166187589632
  %557 = ashr exact i64 %sext199, 32
  %558 = getelementptr inbounds float, float* %216, i64 %557
  %559 = load float, float* %558, align 4, !tbaa !317
  %560 = insertelement <8 x float> %555, float %559, i32 1
  %561 = shl i64 %258, 32
  %sext200 = add i64 %561, 20061792239616
  %562 = ashr exact i64 %sext200, 32
  %563 = getelementptr inbounds float, float* %216, i64 %562
  %564 = load float, float* %563, align 4, !tbaa !317
  %565 = insertelement <8 x float> %560, float %564, i32 2
  %566 = shl i64 %258, 32
  %sext201 = add i64 %566, 29957396889600
  %567 = ashr exact i64 %sext201, 32
  %568 = getelementptr inbounds float, float* %216, i64 %567
  %569 = load float, float* %568, align 4, !tbaa !317
  %570 = insertelement <8 x float> %565, float %569, i32 3
  %571 = shl i64 %258, 32
  %sext202 = add i64 %571, 39853001539584
  %572 = ashr exact i64 %sext202, 32
  %573 = getelementptr inbounds float, float* %216, i64 %572
  %574 = load float, float* %573, align 4, !tbaa !317
  %575 = insertelement <8 x float> %570, float %574, i32 4
  %576 = shl i64 %258, 32
  %sext203 = add i64 %576, 49748606189568
  %577 = ashr exact i64 %sext203, 32
  %578 = getelementptr inbounds float, float* %216, i64 %577
  %579 = load float, float* %578, align 4, !tbaa !317
  %580 = insertelement <8 x float> %575, float %579, i32 5
  %581 = shl i64 %258, 32
  %sext204 = add i64 %581, 59644210839552
  %582 = ashr exact i64 %sext204, 32
  %583 = getelementptr inbounds float, float* %216, i64 %582
  %584 = load float, float* %583, align 4, !tbaa !317
  %585 = insertelement <8 x float> %580, float %584, i32 6
  %586 = shl i64 %258, 32
  %sext205 = add i64 %586, 69539815489536
  %587 = ashr exact i64 %sext205, 32
  %588 = getelementptr inbounds float, float* %216, i64 %587
  %589 = load float, float* %588, align 4, !tbaa !317
  %590 = insertelement <8 x float> %585, float %589, i32 7
  %591 = getelementptr inbounds [147456 x <8 x float>], [147456 x <8 x float>]* %4, i64 0, i64 0, i64 %551
  %592 = bitcast float* %591 to <8 x float>*
  store <8 x float> %590, <8 x float>* %592, align 16, !tbaa !320
  %indvars.iv.next129 = add nuw nsw i64 %indvars.iv128, 1
  %exitcond130 = icmp eq i64 %indvars.iv.next129, 3
  br i1 %exitcond130, label %for_end15, label %for_begin16.preheader, !prof !28

for_end15:                                        ; preds = %for_begin16.preheader
  %indvars.iv.next132 = add nuw nsw i64 %indvars.iv131, 1
  %exitcond133 = icmp eq i64 %indvars.iv.next132, 32
  br i1 %exitcond133, label %for_end12, label %for_begin13.preheader, !prof !28

for_body20:                                       ; preds = %for_end36, %for_begin19.preheader
  %593 = phi i32 [ 0, %for_begin19.preheader ], [ %732, %for_end36 ]
  %594 = urem i32 %593, 12
  %595 = udiv i32 %593, 12
  %596 = mul nsw i32 %595, 18432
  %597 = zext i32 %596 to i64
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %250, i8 0, i64 384, i1 false)
  br label %for_begin25.preheader

for_end21:                                        ; preds = %for_end36
  ret void

for_begin34.preheader:                            ; preds = %for_end27
  store <8 x float> %850, <8 x float>* %.sub, align 16, !tbaa !323
  store <8 x float> %856, <8 x float>* %228, align 16, !tbaa !323
  store <8 x float> %862, <8 x float>* %230, align 16, !tbaa !323
  store <8 x float> %868, <8 x float>* %232, align 16, !tbaa !323
  store <8 x float> %874, <8 x float>* %234, align 16, !tbaa !323
  store <8 x float> %880, <8 x float>* %236, align 16, !tbaa !323
  store <8 x float> %886, <8 x float>* %238, align 16, !tbaa !323
  store <8 x float> %892, <8 x float>* %240, align 16, !tbaa !323
  store <8 x float> %898, <8 x float>* %242, align 16, !tbaa !323
  store <8 x float> %904, <8 x float>* %244, align 16, !tbaa !323
  store <8 x float> %910, <8 x float>* %246, align 16, !tbaa !323
  store <8 x float> %916, <8 x float>* %248, align 16, !tbaa !323
  %598 = mul nuw nsw i32 %594, 12
  %599 = mul nsw i32 %595, 1152
  %600 = add nuw nsw i32 %599, %598
  %601 = zext i32 %600 to i64
  br label %for_body35

for_begin25.preheader:                            ; preds = %for_end27, %for_body20
  %indvars.iv118 = phi i64 [ 0, %for_body20 ], [ %indvars.iv.next119, %for_end27 ]
  %.lcssa23.lcssa.lcssa91 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %916, %for_end27 ]
  %.lcssa21.lcssa.lcssa89 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %910, %for_end27 ]
  %.lcssa19.lcssa.lcssa87 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %904, %for_end27 ]
  %.lcssa17.lcssa.lcssa85 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %898, %for_end27 ]
  %.lcssa15.lcssa.lcssa83 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %892, %for_end27 ]
  %.lcssa13.lcssa.lcssa81 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %886, %for_end27 ]
  %.lcssa11.lcssa.lcssa79 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %880, %for_end27 ]
  %.lcssa9.lcssa.lcssa77 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %874, %for_end27 ]
  %.lcssa7.lcssa.lcssa76 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %868, %for_end27 ]
  %.lcssa5.lcssa.lcssa74 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %862, %for_end27 ]
  %.lcssa3.lcssa.lcssa72 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %856, %for_end27 ]
  %.lcssa.lcssa.lcssa70 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %850, %for_end27 ]
  %602 = mul nuw nsw i64 %indvars.iv118, 576
  %603 = add nuw nsw i64 %602, %597
  %604 = trunc i64 %indvars.iv118 to i32
  %605 = mul i32 %604, 1568
  br label %for_begin28.preheader

for_begin28.preheader:                            ; preds = %for_end33.2, %for_begin25.preheader
  %indvars.iv114 = phi i64 [ 0, %for_begin25.preheader ], [ %indvars.iv.next115, %for_end33.2 ]
  %.lcssa23.lcssa68 = phi <8 x float> [ %.lcssa23.lcssa.lcssa91, %for_begin25.preheader ], [ %916, %for_end33.2 ]
  %.lcssa21.lcssa66 = phi <8 x float> [ %.lcssa21.lcssa.lcssa89, %for_begin25.preheader ], [ %910, %for_end33.2 ]
  %.lcssa19.lcssa64 = phi <8 x float> [ %.lcssa19.lcssa.lcssa87, %for_begin25.preheader ], [ %904, %for_end33.2 ]
  %.lcssa17.lcssa62 = phi <8 x float> [ %.lcssa17.lcssa.lcssa85, %for_begin25.preheader ], [ %898, %for_end33.2 ]
  %.lcssa15.lcssa60 = phi <8 x float> [ %.lcssa15.lcssa.lcssa83, %for_begin25.preheader ], [ %892, %for_end33.2 ]
  %.lcssa13.lcssa58 = phi <8 x float> [ %.lcssa13.lcssa.lcssa81, %for_begin25.preheader ], [ %886, %for_end33.2 ]
  %.lcssa11.lcssa56 = phi <8 x float> [ %.lcssa11.lcssa.lcssa79, %for_begin25.preheader ], [ %880, %for_end33.2 ]
  %.lcssa9.lcssa54 = phi <8 x float> [ %.lcssa9.lcssa.lcssa77, %for_begin25.preheader ], [ %874, %for_end33.2 ]
  %.lcssa7.lcssa52 = phi <8 x float> [ %.lcssa7.lcssa.lcssa76, %for_begin25.preheader ], [ %868, %for_end33.2 ]
  %.lcssa5.lcssa51 = phi <8 x float> [ %.lcssa5.lcssa.lcssa74, %for_begin25.preheader ], [ %862, %for_end33.2 ]
  %.lcssa3.lcssa49 = phi <8 x float> [ %.lcssa3.lcssa.lcssa72, %for_begin25.preheader ], [ %856, %for_end33.2 ]
  %.lcssa.lcssa47 = phi <8 x float> [ %.lcssa.lcssa.lcssa70, %for_begin25.preheader ], [ %850, %for_end33.2 ]
  %606 = phi i32 [ 0, %for_begin25.preheader ], [ %917, %for_end33.2 ]
  %reass.add = add nuw nsw i32 %606, %594
  %reass.mul = mul i32 %reass.add, 112
  %607 = add nsw i32 %reass.mul, %605
  %608 = mul nuw nsw i64 %indvars.iv114, 192
  %609 = add nuw nsw i64 %603, %608
  %610 = sext i32 %607 to i64
  br label %for_body32

for_end27:                                        ; preds = %for_end33.2
  %indvars.iv.next119 = add nuw nsw i64 %indvars.iv118, 1
  %exitcond120 = icmp eq i64 %indvars.iv.next119, 32
  br i1 %exitcond120, label %for_begin34.preheader, label %for_begin25.preheader, !prof !28

for_body32:                                       ; preds = %for_body32, %for_begin28.preheader
  %indvars.iv = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next, %for_body32 ]
  %611 = phi <8 x float> [ %.lcssa23.lcssa68, %for_begin28.preheader ], [ %701, %for_body32 ]
  %612 = phi <8 x float> [ %.lcssa21.lcssa66, %for_begin28.preheader ], [ %695, %for_body32 ]
  %613 = phi <8 x float> [ %.lcssa19.lcssa64, %for_begin28.preheader ], [ %689, %for_body32 ]
  %614 = phi <8 x float> [ %.lcssa17.lcssa62, %for_begin28.preheader ], [ %683, %for_body32 ]
  %615 = phi <8 x float> [ %.lcssa15.lcssa60, %for_begin28.preheader ], [ %677, %for_body32 ]
  %616 = phi <8 x float> [ %.lcssa13.lcssa58, %for_begin28.preheader ], [ %671, %for_body32 ]
  %617 = phi <8 x float> [ %.lcssa11.lcssa56, %for_begin28.preheader ], [ %665, %for_body32 ]
  %618 = phi <8 x float> [ %.lcssa9.lcssa54, %for_begin28.preheader ], [ %659, %for_body32 ]
  %619 = phi <8 x float> [ %.lcssa7.lcssa52, %for_begin28.preheader ], [ %653, %for_body32 ]
  %620 = phi <8 x float> [ %.lcssa5.lcssa51, %for_begin28.preheader ], [ %647, %for_body32 ]
  %621 = phi <8 x float> [ %.lcssa3.lcssa49, %for_begin28.preheader ], [ %641, %for_body32 ]
  %622 = phi <8 x float> [ %.lcssa.lcssa47, %for_begin28.preheader ], [ %635, %for_body32 ]
  %623 = mul nuw nsw i64 %indvars.iv, 14
  %624 = add nsw i64 %623, %610
  %625 = and i64 %624, 4294967294
  %626 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %625
  %627 = load float, float* %626, align 8, !tbaa !311
  %628 = insertelement <8 x float> undef, float %627, i32 0
  %629 = shufflevector <8 x float> %628, <8 x float> undef, <8 x i32> zeroinitializer
  %630 = shl i64 %indvars.iv, 3
  %631 = add nuw nsw i64 %609, %630
  %632 = getelementptr inbounds [147456 x <8 x float>], [147456 x <8 x float>]* %4, i64 0, i64 0, i64 %631
  %633 = bitcast float* %632 to <8 x float>*
  %634 = load <8 x float>, <8 x float>* %633, align 16, !tbaa !320
  %635 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %629, <8 x float> %634, <8 x float> %622)
  %636 = or i64 %624, 1
  %637 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %636
  %638 = load float, float* %637, align 4, !tbaa !311
  %639 = insertelement <8 x float> undef, float %638, i32 0
  %640 = shufflevector <8 x float> %639, <8 x float> undef, <8 x i32> zeroinitializer
  %641 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %640, <8 x float> %634, <8 x float> %621)
  %642 = add nuw nsw i64 %624, 2
  %643 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %642
  %644 = load float, float* %643, align 8, !tbaa !311
  %645 = insertelement <8 x float> undef, float %644, i32 0
  %646 = shufflevector <8 x float> %645, <8 x float> undef, <8 x i32> zeroinitializer
  %647 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %646, <8 x float> %634, <8 x float> %620)
  %648 = add nuw nsw i64 %624, 3
  %649 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %648
  %650 = load float, float* %649, align 4, !tbaa !311
  %651 = insertelement <8 x float> undef, float %650, i32 0
  %652 = shufflevector <8 x float> %651, <8 x float> undef, <8 x i32> zeroinitializer
  %653 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %652, <8 x float> %634, <8 x float> %619)
  %654 = add nuw nsw i64 %624, 4
  %655 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %654
  %656 = load float, float* %655, align 8, !tbaa !311
  %657 = insertelement <8 x float> undef, float %656, i32 0
  %658 = shufflevector <8 x float> %657, <8 x float> undef, <8 x i32> zeroinitializer
  %659 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %658, <8 x float> %634, <8 x float> %618)
  %660 = add nuw nsw i64 %624, 5
  %661 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %660
  %662 = load float, float* %661, align 4, !tbaa !311
  %663 = insertelement <8 x float> undef, float %662, i32 0
  %664 = shufflevector <8 x float> %663, <8 x float> undef, <8 x i32> zeroinitializer
  %665 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %664, <8 x float> %634, <8 x float> %617)
  %666 = add nuw nsw i64 %624, 6
  %667 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %666
  %668 = load float, float* %667, align 8, !tbaa !311
  %669 = insertelement <8 x float> undef, float %668, i32 0
  %670 = shufflevector <8 x float> %669, <8 x float> undef, <8 x i32> zeroinitializer
  %671 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %670, <8 x float> %634, <8 x float> %616)
  %672 = add nuw nsw i64 %624, 7
  %673 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %672
  %674 = load float, float* %673, align 4, !tbaa !311
  %675 = insertelement <8 x float> undef, float %674, i32 0
  %676 = shufflevector <8 x float> %675, <8 x float> undef, <8 x i32> zeroinitializer
  %677 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %676, <8 x float> %634, <8 x float> %615)
  %678 = add nuw nsw i64 %624, 8
  %679 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %678
  %680 = load float, float* %679, align 8, !tbaa !311
  %681 = insertelement <8 x float> undef, float %680, i32 0
  %682 = shufflevector <8 x float> %681, <8 x float> undef, <8 x i32> zeroinitializer
  %683 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %682, <8 x float> %634, <8 x float> %614)
  %684 = add nuw nsw i64 %624, 9
  %685 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %684
  %686 = load float, float* %685, align 4, !tbaa !311
  %687 = insertelement <8 x float> undef, float %686, i32 0
  %688 = shufflevector <8 x float> %687, <8 x float> undef, <8 x i32> zeroinitializer
  %689 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %688, <8 x float> %634, <8 x float> %613)
  %690 = add nuw nsw i64 %624, 10
  %691 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %690
  %692 = load float, float* %691, align 8, !tbaa !311
  %693 = insertelement <8 x float> undef, float %692, i32 0
  %694 = shufflevector <8 x float> %693, <8 x float> undef, <8 x i32> zeroinitializer
  %695 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %694, <8 x float> %634, <8 x float> %612)
  %696 = add nuw nsw i64 %624, 11
  %697 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %696
  %698 = load float, float* %697, align 4, !tbaa !311
  %699 = insertelement <8 x float> undef, float %698, i32 0
  %700 = shufflevector <8 x float> %699, <8 x float> undef, <8 x i32> zeroinitializer
  %701 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %700, <8 x float> %634, <8 x float> %611)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 8
  br i1 %exitcond, label %for_end33, label %for_body32, !prof !28

for_end33:                                        ; preds = %for_body32
  %702 = or i64 %610, 1
  %703 = add nuw nsw i64 %609, 64
  br label %for_body32.1

for_body35:                                       ; preds = %for_body35, %for_begin34.preheader
  %indvars.iv121 = phi i64 [ 0, %for_begin34.preheader ], [ %indvars.iv.next122, %for_body35 ]
  %704 = add nuw nsw i64 %indvars.iv121, %601
  %705 = add nuw nsw i64 %704, 144
  %706 = add nuw nsw i64 %704, 288
  %707 = add nuw nsw i64 %704, 432
  %708 = add nuw nsw i64 %704, 576
  %709 = add nuw nsw i64 %704, 720
  %710 = add nuw nsw i64 %704, 864
  %711 = add nuw nsw i64 %704, 1008
  %712 = shl nsw i64 %indvars.iv121, 3
  %713 = getelementptr inbounds [12 x <8 x float>], [12 x <8 x float>]* %3, i64 0, i64 0, i64 %712
  %714 = bitcast float* %713 to <8 x float>*
  %715 = load <8 x float>, <8 x float>* %714, align 16, !tbaa !334
  %716 = getelementptr inbounds float, float* %249, i64 %704
  %717 = extractelement <8 x float> %715, i64 0
  store float %717, float* %716, align 4, !tbaa !335
  %718 = getelementptr inbounds float, float* %249, i64 %705
  %719 = extractelement <8 x float> %715, i64 1
  store float %719, float* %718, align 4, !tbaa !335
  %720 = getelementptr inbounds float, float* %249, i64 %706
  %721 = extractelement <8 x float> %715, i64 2
  store float %721, float* %720, align 4, !tbaa !335
  %722 = getelementptr inbounds float, float* %249, i64 %707
  %723 = extractelement <8 x float> %715, i64 3
  store float %723, float* %722, align 4, !tbaa !335
  %724 = getelementptr inbounds float, float* %249, i64 %708
  %725 = extractelement <8 x float> %715, i64 4
  store float %725, float* %724, align 4, !tbaa !335
  %726 = getelementptr inbounds float, float* %249, i64 %709
  %727 = extractelement <8 x float> %715, i64 5
  store float %727, float* %726, align 4, !tbaa !335
  %728 = getelementptr inbounds float, float* %249, i64 %710
  %729 = extractelement <8 x float> %715, i64 6
  store float %729, float* %728, align 4, !tbaa !335
  %730 = getelementptr inbounds float, float* %249, i64 %711
  %731 = extractelement <8 x float> %715, i64 7
  store float %731, float* %730, align 4, !tbaa !335
  %indvars.iv.next122 = add nuw nsw i64 %indvars.iv121, 1
  %exitcond123 = icmp eq i64 %indvars.iv.next122, 12
  br i1 %exitcond123, label %for_end36, label %for_body35, !prof !28

for_end36:                                        ; preds = %for_body35
  %732 = add nuw nsw i32 %593, 1
  %exitcond124 = icmp eq i32 %732, 768
  br i1 %exitcond124, label %for_end21, label %for_body20, !prof !28

for_body32.1:                                     ; preds = %for_body32.1, %for_end33
  %indvars.iv.1 = phi i64 [ 0, %for_end33 ], [ %indvars.iv.next.1, %for_body32.1 ]
  %733 = phi <8 x float> [ %701, %for_end33 ], [ %823, %for_body32.1 ]
  %734 = phi <8 x float> [ %695, %for_end33 ], [ %817, %for_body32.1 ]
  %735 = phi <8 x float> [ %689, %for_end33 ], [ %811, %for_body32.1 ]
  %736 = phi <8 x float> [ %683, %for_end33 ], [ %805, %for_body32.1 ]
  %737 = phi <8 x float> [ %677, %for_end33 ], [ %799, %for_body32.1 ]
  %738 = phi <8 x float> [ %671, %for_end33 ], [ %793, %for_body32.1 ]
  %739 = phi <8 x float> [ %665, %for_end33 ], [ %787, %for_body32.1 ]
  %740 = phi <8 x float> [ %659, %for_end33 ], [ %781, %for_body32.1 ]
  %741 = phi <8 x float> [ %653, %for_end33 ], [ %775, %for_body32.1 ]
  %742 = phi <8 x float> [ %647, %for_end33 ], [ %769, %for_body32.1 ]
  %743 = phi <8 x float> [ %641, %for_end33 ], [ %763, %for_body32.1 ]
  %744 = phi <8 x float> [ %635, %for_end33 ], [ %757, %for_body32.1 ]
  %745 = mul nuw nsw i64 %indvars.iv.1, 14
  %746 = add nsw i64 %702, %745
  %747 = and i64 %746, 4294967295
  %748 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %747
  %749 = load float, float* %748, align 4, !tbaa !311
  %750 = insertelement <8 x float> undef, float %749, i32 0
  %751 = shufflevector <8 x float> %750, <8 x float> undef, <8 x i32> zeroinitializer
  %752 = shl i64 %indvars.iv.1, 3
  %753 = add nuw nsw i64 %703, %752
  %754 = getelementptr inbounds [147456 x <8 x float>], [147456 x <8 x float>]* %4, i64 0, i64 0, i64 %753
  %755 = bitcast float* %754 to <8 x float>*
  %756 = load <8 x float>, <8 x float>* %755, align 16, !tbaa !320
  %757 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %751, <8 x float> %756, <8 x float> %744)
  %758 = add nuw nsw i64 %746, 1
  %759 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %758
  %760 = load float, float* %759, align 8, !tbaa !311
  %761 = insertelement <8 x float> undef, float %760, i32 0
  %762 = shufflevector <8 x float> %761, <8 x float> undef, <8 x i32> zeroinitializer
  %763 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %762, <8 x float> %756, <8 x float> %743)
  %764 = add nuw nsw i64 %746, 2
  %765 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %764
  %766 = load float, float* %765, align 4, !tbaa !311
  %767 = insertelement <8 x float> undef, float %766, i32 0
  %768 = shufflevector <8 x float> %767, <8 x float> undef, <8 x i32> zeroinitializer
  %769 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %768, <8 x float> %756, <8 x float> %742)
  %770 = add nuw nsw i64 %746, 3
  %771 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %770
  %772 = load float, float* %771, align 8, !tbaa !311
  %773 = insertelement <8 x float> undef, float %772, i32 0
  %774 = shufflevector <8 x float> %773, <8 x float> undef, <8 x i32> zeroinitializer
  %775 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %774, <8 x float> %756, <8 x float> %741)
  %776 = add nuw nsw i64 %746, 4
  %777 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %776
  %778 = load float, float* %777, align 4, !tbaa !311
  %779 = insertelement <8 x float> undef, float %778, i32 0
  %780 = shufflevector <8 x float> %779, <8 x float> undef, <8 x i32> zeroinitializer
  %781 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %780, <8 x float> %756, <8 x float> %740)
  %782 = add nuw nsw i64 %746, 5
  %783 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %782
  %784 = load float, float* %783, align 8, !tbaa !311
  %785 = insertelement <8 x float> undef, float %784, i32 0
  %786 = shufflevector <8 x float> %785, <8 x float> undef, <8 x i32> zeroinitializer
  %787 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %786, <8 x float> %756, <8 x float> %739)
  %788 = add nuw nsw i64 %746, 6
  %789 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %788
  %790 = load float, float* %789, align 4, !tbaa !311
  %791 = insertelement <8 x float> undef, float %790, i32 0
  %792 = shufflevector <8 x float> %791, <8 x float> undef, <8 x i32> zeroinitializer
  %793 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %792, <8 x float> %756, <8 x float> %738)
  %794 = add nuw nsw i64 %746, 7
  %795 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %794
  %796 = load float, float* %795, align 8, !tbaa !311
  %797 = insertelement <8 x float> undef, float %796, i32 0
  %798 = shufflevector <8 x float> %797, <8 x float> undef, <8 x i32> zeroinitializer
  %799 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %798, <8 x float> %756, <8 x float> %737)
  %800 = add nuw nsw i64 %746, 8
  %801 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %800
  %802 = load float, float* %801, align 4, !tbaa !311
  %803 = insertelement <8 x float> undef, float %802, i32 0
  %804 = shufflevector <8 x float> %803, <8 x float> undef, <8 x i32> zeroinitializer
  %805 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %804, <8 x float> %756, <8 x float> %736)
  %806 = add nuw nsw i64 %746, 9
  %807 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %806
  %808 = load float, float* %807, align 8, !tbaa !311
  %809 = insertelement <8 x float> undef, float %808, i32 0
  %810 = shufflevector <8 x float> %809, <8 x float> undef, <8 x i32> zeroinitializer
  %811 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %810, <8 x float> %756, <8 x float> %735)
  %812 = add nuw nsw i64 %746, 10
  %813 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %812
  %814 = load float, float* %813, align 4, !tbaa !311
  %815 = insertelement <8 x float> undef, float %814, i32 0
  %816 = shufflevector <8 x float> %815, <8 x float> undef, <8 x i32> zeroinitializer
  %817 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %816, <8 x float> %756, <8 x float> %734)
  %818 = add nuw nsw i64 %746, 11
  %819 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %818
  %820 = load float, float* %819, align 8, !tbaa !311
  %821 = insertelement <8 x float> undef, float %820, i32 0
  %822 = shufflevector <8 x float> %821, <8 x float> undef, <8 x i32> zeroinitializer
  %823 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %822, <8 x float> %756, <8 x float> %733)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 8
  br i1 %exitcond.1, label %for_end33.1, label %for_body32.1, !prof !28

for_end33.1:                                      ; preds = %for_body32.1
  %824 = or i64 %610, 2
  %825 = add nuw nsw i64 %609, 128
  br label %for_body32.2

for_body32.2:                                     ; preds = %for_body32.2, %for_end33.1
  %indvars.iv.2 = phi i64 [ 0, %for_end33.1 ], [ %indvars.iv.next.2, %for_body32.2 ]
  %826 = phi <8 x float> [ %823, %for_end33.1 ], [ %916, %for_body32.2 ]
  %827 = phi <8 x float> [ %817, %for_end33.1 ], [ %910, %for_body32.2 ]
  %828 = phi <8 x float> [ %811, %for_end33.1 ], [ %904, %for_body32.2 ]
  %829 = phi <8 x float> [ %805, %for_end33.1 ], [ %898, %for_body32.2 ]
  %830 = phi <8 x float> [ %799, %for_end33.1 ], [ %892, %for_body32.2 ]
  %831 = phi <8 x float> [ %793, %for_end33.1 ], [ %886, %for_body32.2 ]
  %832 = phi <8 x float> [ %787, %for_end33.1 ], [ %880, %for_body32.2 ]
  %833 = phi <8 x float> [ %781, %for_end33.1 ], [ %874, %for_body32.2 ]
  %834 = phi <8 x float> [ %775, %for_end33.1 ], [ %868, %for_body32.2 ]
  %835 = phi <8 x float> [ %769, %for_end33.1 ], [ %862, %for_body32.2 ]
  %836 = phi <8 x float> [ %763, %for_end33.1 ], [ %856, %for_body32.2 ]
  %837 = phi <8 x float> [ %757, %for_end33.1 ], [ %850, %for_body32.2 ]
  %838 = mul nuw nsw i64 %indvars.iv.2, 14
  %839 = add nsw i64 %824, %838
  %840 = and i64 %839, 4294967294
  %841 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %840
  %842 = load float, float* %841, align 8, !tbaa !311
  %843 = insertelement <8 x float> undef, float %842, i32 0
  %844 = shufflevector <8 x float> %843, <8 x float> undef, <8 x i32> zeroinitializer
  %845 = shl i64 %indvars.iv.2, 3
  %846 = add nuw nsw i64 %825, %845
  %847 = getelementptr inbounds [147456 x <8 x float>], [147456 x <8 x float>]* %4, i64 0, i64 0, i64 %846
  %848 = bitcast float* %847 to <8 x float>*
  %849 = load <8 x float>, <8 x float>* %848, align 16, !tbaa !320
  %850 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %844, <8 x float> %849, <8 x float> %837)
  %851 = or i64 %839, 1
  %852 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %851
  %853 = load float, float* %852, align 4, !tbaa !311
  %854 = insertelement <8 x float> undef, float %853, i32 0
  %855 = shufflevector <8 x float> %854, <8 x float> undef, <8 x i32> zeroinitializer
  %856 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %855, <8 x float> %849, <8 x float> %836)
  %857 = add nuw nsw i64 %839, 2
  %858 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %857
  %859 = load float, float* %858, align 4, !tbaa !311
  %860 = insertelement <8 x float> undef, float %859, i32 0
  %861 = shufflevector <8 x float> %860, <8 x float> undef, <8 x i32> zeroinitializer
  %862 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %861, <8 x float> %849, <8 x float> %835)
  %863 = add nuw nsw i64 %839, 3
  %864 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %863
  %865 = load float, float* %864, align 4, !tbaa !311
  %866 = insertelement <8 x float> undef, float %865, i32 0
  %867 = shufflevector <8 x float> %866, <8 x float> undef, <8 x i32> zeroinitializer
  %868 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %867, <8 x float> %849, <8 x float> %834)
  %869 = add nuw nsw i64 %839, 4
  %870 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %869
  %871 = load float, float* %870, align 4, !tbaa !311
  %872 = insertelement <8 x float> undef, float %871, i32 0
  %873 = shufflevector <8 x float> %872, <8 x float> undef, <8 x i32> zeroinitializer
  %874 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %873, <8 x float> %849, <8 x float> %833)
  %875 = add nuw nsw i64 %839, 5
  %876 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %875
  %877 = load float, float* %876, align 4, !tbaa !311
  %878 = insertelement <8 x float> undef, float %877, i32 0
  %879 = shufflevector <8 x float> %878, <8 x float> undef, <8 x i32> zeroinitializer
  %880 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %879, <8 x float> %849, <8 x float> %832)
  %881 = add nuw nsw i64 %839, 6
  %882 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %881
  %883 = load float, float* %882, align 4, !tbaa !311
  %884 = insertelement <8 x float> undef, float %883, i32 0
  %885 = shufflevector <8 x float> %884, <8 x float> undef, <8 x i32> zeroinitializer
  %886 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %885, <8 x float> %849, <8 x float> %831)
  %887 = add nuw nsw i64 %839, 7
  %888 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %887
  %889 = load float, float* %888, align 4, !tbaa !311
  %890 = insertelement <8 x float> undef, float %889, i32 0
  %891 = shufflevector <8 x float> %890, <8 x float> undef, <8 x i32> zeroinitializer
  %892 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %891, <8 x float> %849, <8 x float> %830)
  %893 = add nuw nsw i64 %839, 8
  %894 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %893
  %895 = load float, float* %894, align 4, !tbaa !311
  %896 = insertelement <8 x float> undef, float %895, i32 0
  %897 = shufflevector <8 x float> %896, <8 x float> undef, <8 x i32> zeroinitializer
  %898 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %897, <8 x float> %849, <8 x float> %829)
  %899 = add nuw nsw i64 %839, 9
  %900 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %899
  %901 = load float, float* %900, align 4, !tbaa !311
  %902 = insertelement <8 x float> undef, float %901, i32 0
  %903 = shufflevector <8 x float> %902, <8 x float> undef, <8 x i32> zeroinitializer
  %904 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %903, <8 x float> %849, <8 x float> %828)
  %905 = add nuw nsw i64 %839, 10
  %906 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %905
  %907 = load float, float* %906, align 4, !tbaa !311
  %908 = insertelement <8 x float> undef, float %907, i32 0
  %909 = shufflevector <8 x float> %908, <8 x float> undef, <8 x i32> zeroinitializer
  %910 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %909, <8 x float> %849, <8 x float> %827)
  %911 = add nuw nsw i64 %839, 11
  %912 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %911
  %913 = load float, float* %912, align 4, !tbaa !311
  %914 = insertelement <8 x float> undef, float %913, i32 0
  %915 = shufflevector <8 x float> %914, <8 x float> undef, <8 x i32> zeroinitializer
  %916 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %915, <8 x float> %849, <8 x float> %826)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 8
  br i1 %exitcond.2, label %for_end33.2, label %for_body32.2, !prof !28

for_end33.2:                                      ; preds = %for_body32.2
  %indvars.iv.next115 = add nuw nsw i64 %indvars.iv114, 1
  %917 = add nuw nsw i32 %606, 1
  %exitcond117 = icmp eq i64 %indvars.iv.next115, 3
  br i1 %exitcond117, label %for_end27, label %for_begin28.preheader, !prof !28
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_bias_add_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_bias_add_2_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_bias_add_2_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %0 to float*
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ]
  %6 = getelementptr inbounds float, float* %3, i64 %index
  %7 = bitcast float* %6 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %7, align 4, !tbaa !338
  %8 = getelementptr inbounds float, float* %6, i64 4
  %9 = bitcast float* %8 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !338
  %10 = getelementptr inbounds float, float* %4, i64 %index
  %11 = bitcast float* %10 to <4 x float>*
  %wide.load3 = load <4 x float>, <4 x float>* %11, align 4, !tbaa !341
  %12 = getelementptr inbounds float, float* %10, i64 4
  %13 = bitcast float* %12 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %13, align 4, !tbaa !341
  %14 = fadd <4 x float> %wide.load, %wide.load3
  %15 = fadd <4 x float> %wide.load2, %wide.load4
  %16 = getelementptr inbounds float, float* %5, i64 %index
  %17 = bitcast float* %16 to <4 x float>*
  store <4 x float> %14, <4 x float>* %17, align 4, !tbaa !344
  %18 = getelementptr inbounds float, float* %16, i64 4
  %19 = bitcast float* %18 to <4 x float>*
  store <4 x float> %15, <4 x float>* %19, align 4, !tbaa !344
  %index.next = add i64 %index, 8
  %20 = icmp eq i64 %index.next, 4096
  br i1 %20, label %for_end, label %vector.body, !llvm.loop !347

for_end:                                          ; preds = %vector.body
  ret void
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_softmax(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
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
  tail call fastcc void @fused_nn_softmax_compute_(i8* %9, i8* %11)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_softmax_compute_(i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #3 {
entry:
  %2 = alloca [1000 x float], align 16
  %3 = bitcast i8* %0 to float*
  br label %for_body

vector.ph:                                        ; preds = %for_body
  %broadcast.splatinsert15 = insertelement <4 x float> undef, float %14, i32 0
  %broadcast.splat16 = shufflevector <4 x float> %broadcast.splatinsert15, <4 x float> undef, <4 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %vector.ph
  %index = phi i64 [ 0, %vector.ph ], [ %index.next, %vector.body ]
  %4 = getelementptr inbounds float, float* %3, i64 %index
  %5 = bitcast float* %4 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %5, align 4, !tbaa !348
  %6 = fsub <4 x float> %wide.load, %broadcast.splat16
  %7 = call <4 x float> @llvm.exp.v4f32(<4 x float> %6)
  %8 = getelementptr inbounds [1000 x float], [1000 x float]* %2, i64 0, i64 %index
  %9 = bitcast float* %8 to <4 x float>*
  store <4 x float> %7, <4 x float>* %9, align 16, !tbaa !351
  %index.next = add i64 %index, 4
  %10 = icmp eq i64 %index.next, 1000
  br i1 %10, label %for_body5, label %vector.body, !llvm.loop !354

for_body:                                         ; preds = %for_body, %entry
  %indvars.iv10 = phi i64 [ 0, %entry ], [ %indvars.iv.next11, %for_body ]
  %.02 = phi float [ 0xC7EFFFFFE0000000, %entry ], [ %14, %for_body ]
  %11 = getelementptr inbounds float, float* %3, i64 %indvars.iv10
  %12 = load float, float* %11, align 4, !tbaa !348
  %13 = fcmp ogt float %.02, %12
  %14 = select i1 %13, float %.02, float %12
  %indvars.iv.next11 = add nuw nsw i64 %indvars.iv10, 1
  %exitcond12 = icmp eq i64 %indvars.iv.next11, 1000
  br i1 %exitcond12, label %vector.ph, label %for_body, !prof !28

for_begin7.preheader:                             ; preds = %for_body5
  %15 = bitcast i8* %1 to float*
  %broadcast.splatinsert29 = insertelement <4 x float> undef, float %24, i32 0
  %broadcast.splat30 = shufflevector <4 x float> %broadcast.splatinsert29, <4 x float> undef, <4 x i32> zeroinitializer
  br label %vector.body17

vector.body17:                                    ; preds = %vector.body17, %for_begin7.preheader
  %index21 = phi i64 [ 0, %for_begin7.preheader ], [ %index.next22, %vector.body17 ]
  %16 = getelementptr inbounds [1000 x float], [1000 x float]* %2, i64 0, i64 %index21
  %17 = bitcast float* %16 to <4 x float>*
  %wide.load28 = load <4 x float>, <4 x float>* %17, align 16, !tbaa !351
  %18 = fdiv <4 x float> %wide.load28, %broadcast.splat30
  %19 = getelementptr inbounds float, float* %15, i64 %index21
  %20 = bitcast float* %19 to <4 x float>*
  store <4 x float> %18, <4 x float>* %20, align 4, !tbaa !355
  %index.next22 = add i64 %index21, 4
  %21 = icmp eq i64 %index.next22, 1000
  br i1 %21, label %for_end9, label %vector.body17, !llvm.loop !358

for_body5:                                        ; preds = %vector.body, %for_body5
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for_body5 ], [ 0, %vector.body ]
  %.0131 = phi float [ %24, %for_body5 ], [ 0.000000e+00, %vector.body ]
  %22 = getelementptr inbounds [1000 x float], [1000 x float]* %2, i64 0, i64 %indvars.iv4
  %23 = load float, float* %22, align 4, !tbaa !351
  %24 = fadd float %.0131, %23
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 1000
  br i1 %exitcond6, label %for_begin7.preheader, label %for_body5, !prof !28

for_end9:                                         ; preds = %vector.body17
  ret void
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_dense_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
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
  tail call fastcc void @fused_nn_dense_1_compute_(i8* %12, i8* %14, i8* %16)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_dense_1_compute_(i8* noalias nocapture readonly, i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #3 {
entry:
  %3 = bitcast i8* %0 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %2 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv3 = phi i64 [ 0, %entry ], [ %indvars.iv.next4, %for_end3 ]
  %6 = shl i64 %indvars.iv3, 12
  br label %for_body2

for_end:                                          ; preds = %for_end3
  ret void

for_body2:                                        ; preds = %for_body2, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_body2 ]
  %.02 = phi <16 x float> [ zeroinitializer, %for_begin1.preheader ], [ %15, %for_body2 ]
  %7 = shl nsw i64 %indvars.iv, 4
  %8 = getelementptr inbounds float, float* %3, i64 %7
  %9 = bitcast float* %8 to <16 x float>*
  %10 = load <16 x float>, <16 x float>* %9, align 64, !tbaa !359
  %11 = add nuw nsw i64 %7, %6
  %12 = getelementptr inbounds float, float* %4, i64 %11
  %13 = bitcast float* %12 to <16 x float>*
  %14 = load <16 x float>, <16 x float>* %13, align 64, !tbaa !362
  %15 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %10, <16 x float> %14, <16 x float> %.02)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2
  %16 = getelementptr inbounds float, float* %5, i64 %indvars.iv3
  %.0.vec.extract = extractelement <16 x float> %15, i32 0
  %17 = fadd float %.0.vec.extract, 0.000000e+00
  %.4.vec.extract = extractelement <16 x float> %15, i32 1
  %18 = fadd float %.4.vec.extract, %17
  %.8.vec.extract = extractelement <16 x float> %15, i32 2
  %19 = fadd float %.8.vec.extract, %18
  %.12.vec.extract = extractelement <16 x float> %15, i32 3
  %20 = fadd float %.12.vec.extract, %19
  %.16.vec.extract = extractelement <16 x float> %15, i32 4
  %21 = fadd float %.16.vec.extract, %20
  %.20.vec.extract = extractelement <16 x float> %15, i32 5
  %22 = fadd float %.20.vec.extract, %21
  %.24.vec.extract = extractelement <16 x float> %15, i32 6
  %23 = fadd float %.24.vec.extract, %22
  %.28.vec.extract = extractelement <16 x float> %15, i32 7
  %24 = fadd float %.28.vec.extract, %23
  %.32.vec.extract = extractelement <16 x float> %15, i32 8
  %25 = fadd float %.32.vec.extract, %24
  %.36.vec.extract = extractelement <16 x float> %15, i32 9
  %26 = fadd float %.36.vec.extract, %25
  %.40.vec.extract = extractelement <16 x float> %15, i32 10
  %27 = fadd float %.40.vec.extract, %26
  %.44.vec.extract = extractelement <16 x float> %15, i32 11
  %28 = fadd float %.44.vec.extract, %27
  %.48.vec.extract = extractelement <16 x float> %15, i32 12
  %29 = fadd float %.48.vec.extract, %28
  %.52.vec.extract = extractelement <16 x float> %15, i32 13
  %30 = fadd float %.52.vec.extract, %29
  %.56.vec.extract = extractelement <16 x float> %15, i32 14
  %31 = fadd float %.56.vec.extract, %30
  %.60.vec.extract = extractelement <16 x float> %15, i32 15
  %32 = fadd float %.60.vec.extract, %31
  store float %32, float* %16, align 4, !tbaa !365
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 1024
  br i1 %exitcond5, label %for_end, label %for_begin1.preheader, !prof !28
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_dense_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
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
  tail call fastcc void @fused_nn_dense_2_compute_(i8* %12, i8* %14, i8* %16)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_dense_2_compute_(i8* noalias nocapture readonly, i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #3 {
entry:
  %3 = bitcast i8* %0 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %2 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv3 = phi i64 [ 0, %entry ], [ %indvars.iv.next4, %for_end3 ]
  %6 = mul nuw nsw i64 %indvars.iv3, 18432
  br label %for_body2

for_end:                                          ; preds = %for_end3
  ret void

for_body2:                                        ; preds = %for_body2, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_body2 ]
  %.02 = phi <16 x float> [ zeroinitializer, %for_begin1.preheader ], [ %15, %for_body2 ]
  %7 = shl nsw i64 %indvars.iv, 4
  %8 = getelementptr inbounds float, float* %3, i64 %7
  %9 = bitcast float* %8 to <16 x float>*
  %10 = load <16 x float>, <16 x float>* %9, align 64, !tbaa !368
  %11 = add nuw nsw i64 %7, %6
  %12 = getelementptr inbounds float, float* %4, i64 %11
  %13 = bitcast float* %12 to <16 x float>*
  %14 = load <16 x float>, <16 x float>* %13, align 64, !tbaa !371
  %15 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %10, <16 x float> %14, <16 x float> %.02)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 1152
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !28

for_end3:                                         ; preds = %for_body2
  %16 = getelementptr inbounds float, float* %5, i64 %indvars.iv3
  %.0.vec.extract = extractelement <16 x float> %15, i32 0
  %17 = fadd float %.0.vec.extract, 0.000000e+00
  %.4.vec.extract = extractelement <16 x float> %15, i32 1
  %18 = fadd float %.4.vec.extract, %17
  %.8.vec.extract = extractelement <16 x float> %15, i32 2
  %19 = fadd float %.8.vec.extract, %18
  %.12.vec.extract = extractelement <16 x float> %15, i32 3
  %20 = fadd float %.12.vec.extract, %19
  %.16.vec.extract = extractelement <16 x float> %15, i32 4
  %21 = fadd float %.16.vec.extract, %20
  %.20.vec.extract = extractelement <16 x float> %15, i32 5
  %22 = fadd float %.20.vec.extract, %21
  %.24.vec.extract = extractelement <16 x float> %15, i32 6
  %23 = fadd float %.24.vec.extract, %22
  %.28.vec.extract = extractelement <16 x float> %15, i32 7
  %24 = fadd float %.28.vec.extract, %23
  %.32.vec.extract = extractelement <16 x float> %15, i32 8
  %25 = fadd float %.32.vec.extract, %24
  %.36.vec.extract = extractelement <16 x float> %15, i32 9
  %26 = fadd float %.36.vec.extract, %25
  %.40.vec.extract = extractelement <16 x float> %15, i32 10
  %27 = fadd float %.40.vec.extract, %26
  %.44.vec.extract = extractelement <16 x float> %15, i32 11
  %28 = fadd float %.44.vec.extract, %27
  %.48.vec.extract = extractelement <16 x float> %15, i32 12
  %29 = fadd float %.48.vec.extract, %28
  %.52.vec.extract = extractelement <16 x float> %15, i32 13
  %30 = fadd float %.52.vec.extract, %29
  %.56.vec.extract = extractelement <16 x float> %15, i32 14
  %31 = fadd float %.56.vec.extract, %30
  %.60.vec.extract = extractelement <16 x float> %15, i32 15
  %32 = fadd float %.60.vec.extract, %31
  store float %32, float* %16, align 4, !tbaa !374
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 4096
  br i1 %exitcond5, label %for_end, label %for_begin1.preheader, !prof !28
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_multiply_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_multiply_2_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_multiply_2_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = load float, float* %4, align 64, !tbaa !377
  %6 = bitcast i8* %0 to float*
  %broadcast.splatinsert3 = insertelement <4 x float> undef, float %5, i32 0
  %broadcast.splat4 = shufflevector <4 x float> %broadcast.splatinsert3, <4 x float> undef, <4 x i32> zeroinitializer
  %broadcast.splatinsert5 = insertelement <4 x float> undef, float %5, i32 0
  %broadcast.splat6 = shufflevector <4 x float> %broadcast.splatinsert5, <4 x float> undef, <4 x i32> zeroinitializer
  br label %vector.body

vector.body:                                      ; preds = %vector.body, %entry
  %index = phi i64 [ 0, %entry ], [ %index.next, %vector.body ]
  %7 = getelementptr inbounds float, float* %3, i64 %index
  %8 = bitcast float* %7 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !391
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !391
  %11 = fmul <4 x float> %wide.load, %broadcast.splat4
  %12 = fmul <4 x float> %wide.load2, %broadcast.splat6
  %13 = getelementptr inbounds float, float* %6, i64 %index
  %14 = bitcast float* %13 to <4 x float>*
  store <4 x float> %11, <4 x float>* %14, align 4, !tbaa !394
  %15 = getelementptr inbounds float, float* %13, i64 4
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %12, <4 x float>* %16, align 4, !tbaa !394
  %index.next = add i64 %index, 8
  %17 = icmp eq i64 %index.next, 18432
  br i1 %17, label %for_end, label %vector.body, !llvm.loop !397

for_end:                                          ; preds = %vector.body
  ret void
}

; Function Attrs: argmemonly nounwind
declare void @llvm.memcpy.p0i8.p0i8.i64(i8* nocapture writeonly, i8* nocapture readonly, i64, i1) #5

; Function Attrs: argmemonly nounwind
declare void @llvm.memset.p0i8.i64(i8* nocapture writeonly, i8, i64, i1) #5

; Function Attrs: nounwind readnone speculatable
declare <4 x float> @llvm.fmuladd.v4f32(<4 x float>, <4 x float>, <4 x float>) #4

; Function Attrs: nounwind readnone speculatable
declare <4 x float> @llvm.pow.v4f32(<4 x float>, <4 x float>) #4

; Function Attrs: nounwind readnone speculatable
declare <4 x float> @llvm.exp.v4f32(<4 x float>) #4

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
!6 = !{!"0x3448840.w1.b0", !7, i64 0}
!7 = !{!"0x3448840.w2.b0", !8, i64 0}
!8 = !{!"0x3448840.w4.b0", !9, i64 0}
!9 = !{!"0x3448840.w8.b0", !10, i64 0}
!10 = !{!"0x3448840.w16.b0", !11, i64 0}
!11 = !{!"0x3448840.w32.b0", !12, i64 0}
!12 = !{!"0x3448840.w64.b0", !13, i64 0}
!13 = !{!"0x3448840.w128.b0", !14, i64 0}
!14 = !{!"0x3448840.w256.b0", !15, i64 0}
!15 = !{!"0x3448840.w512.b0", !16, i64 0}
!16 = !{!"0x3448840.w1024.b0", !17, i64 0}
!17 = !{!"float32", !18, i64 0}
!18 = !{!"0x3448840", !19, i64 0}
!19 = !{!"tvm-tbaa"}
!20 = !{!21, !21, i64 0}
!21 = !{!"float32", !22, i64 0}
!22 = !{!"0x3448a40", !19, i64 0}
!23 = !{!24, !24, i64 0}
!24 = !{!"float32", !25, i64 0}
!25 = !{!"0x3448a80", !19, i64 0}
!26 = distinct !{!26, !27}
!27 = !{!"llvm.loop.isvectorized", i32 1}
!28 = !{!"branch_weights", i32 1, i32 1048576}
!29 = !{!30, !30, i64 0}
!30 = !{!"float32", !31, i64 0}
!31 = !{!"0x341ef10", !19, i64 0}
!32 = !{!33, !33, i64 0}
!33 = !{!"float32", !34, i64 0}
!34 = !{!"0x3421770", !19, i64 0}
!35 = !{!36, !36, i64 0}
!36 = !{!"float32", !37, i64 0}
!37 = !{!"0x3427970", !19, i64 0}
!38 = !{!39, !39, i64 0}
!39 = !{!"float32", !40, i64 0}
!40 = !{!"0x3427570", !19, i64 0}
!41 = !{!42, !42, i64 0}
!42 = !{!"float32", !43, i64 0}
!43 = !{!"0x341ee10", !19, i64 0}
!44 = !{!45, !45, i64 0}
!45 = !{!"0x3434a70.w1.b0", !46, i64 0}
!46 = !{!"0x3434a70.w2.b0", !47, i64 0}
!47 = !{!"0x3434a70.w4.b0", !48, i64 0}
!48 = !{!"0x3434a70.w8.b0", !49, i64 0}
!49 = !{!"0x3434a70.w16.b0", !50, i64 0}
!50 = !{!"0x3434a70.w32.b0", !51, i64 0}
!51 = !{!"0x3434a70.w64.b0", !52, i64 0}
!52 = !{!"0x3434a70.w128.b0", !53, i64 0}
!53 = !{!"0x3434a70.w256.b0", !54, i64 0}
!54 = !{!"0x3434a70.w512.b0", !55, i64 0}
!55 = !{!"0x3434a70.w1024.b0", !56, i64 0}
!56 = !{!"float32", !57, i64 0}
!57 = !{!"0x3434a70", !19, i64 0}
!58 = !{!59, !59, i64 0}
!59 = !{!"float32", !60, i64 0}
!60 = !{!"0x3435020", !19, i64 0}
!61 = !{!62, !62, i64 0}
!62 = !{!"float32", !63, i64 0}
!63 = !{!"0x3435060", !19, i64 0}
!64 = distinct !{!64, !27}
!65 = !{!66, !66, i64 0}
!66 = !{!"float32", !67, i64 0}
!67 = !{!"0x33e1610", !19, i64 0}
!68 = !{!69, !69, i64 0}
!69 = !{!"float32", !70, i64 0}
!70 = !{!"0x33e0d10", !19, i64 0}
!71 = distinct !{!71, !27}
!72 = !{!73, !73, i64 0}
!73 = !{!"float32", !74, i64 0}
!74 = !{!"0x3400720", !19, i64 0}
!75 = !{!76, !76, i64 0}
!76 = !{!"float32", !77, i64 0}
!77 = !{!"0x33fec50", !19, i64 0}
!78 = distinct !{!78, !27}
!79 = !{!80, !80, i64 0}
!80 = !{!"float32", !81, i64 0}
!81 = !{!"0x3400990", !19, i64 0}
!82 = !{!83, !83, i64 0}
!83 = !{!"float32", !84, i64 0}
!84 = !{!"0x3400910", !19, i64 0}
!85 = !{!86, !86, i64 0}
!86 = !{!"float32", !87, i64 0}
!87 = !{!"0x33d5050", !19, i64 0}
!88 = !{!89, !89, i64 0}
!89 = !{!"float32", !90, i64 0}
!90 = !{!"0x33d3df0", !19, i64 0}
!91 = !{!92, !92, i64 0}
!92 = !{!"float32", !93, i64 0}
!93 = !{!"0x33d4fc0", !19, i64 0}
!94 = !{!95, !95, i64 0}
!95 = !{!"float32", !96, i64 0}
!96 = !{!"0x33cc9f0", !19, i64 0}
!97 = !{!98, !98, i64 0}
!98 = !{!"float32", !99, i64 0}
!99 = !{!"0x33cd6b0", !19, i64 0}
!100 = !{!101, !101, i64 0}
!101 = !{!"float32", !102, i64 0}
!102 = !{!"0x33c44e0", !19, i64 0}
!103 = !{!104, !104, i64 0}
!104 = !{!"float32", !105, i64 0}
!105 = !{!"0x33c3510", !19, i64 0}
!106 = !{!107, !107, i64 0}
!107 = !{!"float32", !108, i64 0}
!108 = !{!"0x33c41b0", !19, i64 0}
!109 = !{!110, !110, i64 0}
!110 = !{!"float32", !111, i64 0}
!111 = !{!"0x33c43a0", !19, i64 0}
!112 = !{!113, !113, i64 0}
!113 = !{!"float32", !114, i64 0}
!114 = !{!"0x3410d30", !19, i64 0}
!115 = !{!116, !116, i64 0}
!116 = !{!"float32", !117, i64 0}
!117 = !{!"0x340fad0", !19, i64 0}
!118 = !{!119, !119, i64 0}
!119 = !{!"float32", !120, i64 0}
!120 = !{!"0x3410ca0", !19, i64 0}
!121 = !{!122, !122, i64 0}
!122 = !{!"float32", !123, i64 0}
!123 = !{!"0x3408b30", !19, i64 0}
!124 = !{!125, !125, i64 0}
!125 = !{!"float32", !126, i64 0}
!126 = !{!"0x34098c0", !19, i64 0}
!127 = !{!128, !128, i64 0}
!128 = !{!"float32", !129, i64 0}
!129 = !{!"0x33bbd10", !19, i64 0}
!130 = !{!131, !131, i64 0}
!131 = !{!"float32", !132, i64 0}
!132 = !{!"0x33bbcd0", !19, i64 0}
!133 = !{!134, !134, i64 0}
!134 = !{!"float32", !135, i64 0}
!135 = !{!"0x338b5c0", !19, i64 0}
!136 = !{!137, !137, i64 0}
!137 = !{!"float32", !138, i64 0}
!138 = !{!"0x338b740", !19, i64 0}
!139 = !{!140, !140, i64 0}
!140 = !{!"float32", !141, i64 0}
!141 = !{!"0x3394dd0", !19, i64 0}
!142 = !{!143, !143, i64 0}
!143 = !{!"float32", !144, i64 0}
!144 = !{!"0x338e2b0", !19, i64 0}
!145 = !{!146, !146, i64 0}
!146 = !{!"0x33947d0.w8.b0", !147, i64 0}
!147 = !{!"0x33947d0.w16.b0", !148, i64 0}
!148 = !{!"0x33947d0.w32.b0", !149, i64 0}
!149 = !{!"0x33947d0.w64.b0", !150, i64 0}
!150 = !{!"0x33947d0.w128.b0", !151, i64 0}
!151 = !{!"0x33947d0.w256.b0", !152, i64 0}
!152 = !{!"0x33947d0.w512.b0", !153, i64 0}
!153 = !{!"0x33947d0.w1024.b0", !154, i64 0}
!154 = !{!"float32", !155, i64 0}
!155 = !{!"0x33947d0", !19, i64 0}
!156 = !{!154, !154, i64 0}
!157 = !{!158, !158, i64 0}
!158 = !{!"float32", !159, i64 0}
!159 = !{!"0x338b600", !19, i64 0}
!160 = !{!161, !161, i64 0}
!161 = !{!"float32", !162, i64 0}
!162 = !{!"0x33772b0", !19, i64 0}
!163 = !{!164, !164, i64 0}
!164 = !{!"float32", !165, i64 0}
!165 = !{!"0x3376050", !19, i64 0}
!166 = !{!167, !167, i64 0}
!167 = !{!"float32", !168, i64 0}
!168 = !{!"0x3377220", !19, i64 0}
!169 = !{!170, !170, i64 0}
!170 = !{!"float32", !171, i64 0}
!171 = !{!"0x33689e0", !19, i64 0}
!172 = !{!173, !173, i64 0}
!173 = !{!"float32", !174, i64 0}
!174 = !{!"0x33689a0", !19, i64 0}
!175 = !{!176, !176, i64 0}
!176 = !{!"float32", !177, i64 0}
!177 = !{!"0x3314350", !19, i64 0}
!178 = !{!179, !179, i64 0}
!179 = !{!"float32", !180, i64 0}
!180 = !{!"0x3313f50", !19, i64 0}
!181 = distinct !{!181, !27}
!182 = !{!183, !183, i64 0}
!183 = !{!"0x343ea00.w1.b0", !184, i64 0}
!184 = !{!"0x343ea00.w2.b0", !185, i64 0}
!185 = !{!"0x343ea00.w4.b0", !186, i64 0}
!186 = !{!"0x343ea00.w8.b0", !187, i64 0}
!187 = !{!"0x343ea00.w16.b0", !188, i64 0}
!188 = !{!"0x343ea00.w32.b0", !189, i64 0}
!189 = !{!"0x343ea00.w64.b0", !190, i64 0}
!190 = !{!"0x343ea00.w128.b0", !191, i64 0}
!191 = !{!"0x343ea00.w256.b0", !192, i64 0}
!192 = !{!"0x343ea00.w512.b0", !193, i64 0}
!193 = !{!"0x343ea00.w1024.b0", !194, i64 0}
!194 = !{!"float32", !195, i64 0}
!195 = !{!"0x343ea00", !19, i64 0}
!196 = !{!197, !197, i64 0}
!197 = !{!"float32", !198, i64 0}
!198 = !{!"0x343ec90", !19, i64 0}
!199 = !{!200, !200, i64 0}
!200 = !{!"float32", !201, i64 0}
!201 = !{!"0x343ecd0", !19, i64 0}
!202 = distinct !{!202, !27}
!203 = !{!204, !204, i64 0}
!204 = !{!"float32", !205, i64 0}
!205 = !{!"0x3388770", !19, i64 0}
!206 = !{!207, !207, i64 0}
!207 = !{!"float32", !208, i64 0}
!208 = !{!"0x33e3340", !19, i64 0}
!209 = !{!210, !210, i64 0}
!210 = !{!"0x33e5480.w8.b0", !211, i64 0}
!211 = !{!"0x33e5480.w16.b0", !212, i64 0}
!212 = !{!"0x33e5480.w32.b0", !213, i64 0}
!213 = !{!"0x33e5480.w64.b0", !214, i64 0}
!214 = !{!"0x33e5480.w128.b0", !215, i64 0}
!215 = !{!"0x33e5480.w256.b0", !216, i64 0}
!216 = !{!"0x33e5480.w512.b0", !217, i64 0}
!217 = !{!"0x33e5480.w1024.b0", !218, i64 0}
!218 = !{!"float32", !219, i64 0}
!219 = !{!"0x33e5480", !19, i64 0}
!220 = !{!221, !221, i64 0}
!221 = !{!"float32", !222, i64 0}
!222 = !{!"0x3389c40", !19, i64 0}
!223 = !{!218, !218, i64 0}
!224 = !{!225, !225, i64 0}
!225 = !{!"float32", !226, i64 0}
!226 = !{!"0x3375ca0", !19, i64 0}
!227 = !{!228, !228, i64 0}
!228 = !{!"0x32f91f0.w1.b0", !229, i64 0}
!229 = !{!"0x32f91f0.w2.b0", !230, i64 0}
!230 = !{!"0x32f91f0.w4.b0", !231, i64 0}
!231 = !{!"0x32f91f0.w8.b0", !232, i64 0}
!232 = !{!"0x32f91f0.w16.b0", !233, i64 0}
!233 = !{!"0x32f91f0.w32.b0", !234, i64 0}
!234 = !{!"0x32f91f0.w64.b0", !235, i64 0}
!235 = !{!"0x32f91f0.w128.b0", !236, i64 0}
!236 = !{!"0x32f91f0.w256.b0", !237, i64 0}
!237 = !{!"0x32f91f0.w512.b0", !238, i64 0}
!238 = !{!"0x32f91f0.w1024.b0", !239, i64 0}
!239 = !{!"float32", !240, i64 0}
!240 = !{!"0x32f91f0", !19, i64 0}
!241 = !{!242, !242, i64 0}
!242 = !{!"float32", !243, i64 0}
!243 = !{!"0x310e5c0", !19, i64 0}
!244 = !{!245, !245, i64 0}
!245 = !{!"float32", !246, i64 0}
!246 = !{!"0x32fddb0", !19, i64 0}
!247 = distinct !{!247, !27}
!248 = !{!249, !249, i64 0}
!249 = !{!"float32", !250, i64 0}
!250 = !{!"0x32ebe10", !19, i64 0}
!251 = !{!252, !252, i64 0}
!252 = !{!"float32", !253, i64 0}
!253 = !{!"0x32fa8b0", !19, i64 0}
!254 = !{!255, !255, i64 0}
!255 = !{!"float32", !256, i64 0}
!256 = !{!"0x32ebdd0", !19, i64 0}
!257 = distinct !{!257, !27}
!258 = !{!259, !259, i64 0}
!259 = !{!"float32", !260, i64 0}
!260 = !{!"0x32eadf0", !19, i64 0}
!261 = !{!262, !262, i64 0}
!262 = !{!"float32", !263, i64 0}
!263 = !{!"0x32d9c40", !19, i64 0}
!264 = !{!265, !265, i64 0}
!265 = !{!"float32", !266, i64 0}
!266 = !{!"0x32d14d0", !19, i64 0}
!267 = !{!268, !268, i64 0}
!268 = !{!"float32", !269, i64 0}
!269 = !{!"0x32e5050", !19, i64 0}
!270 = !{!271, !271, i64 0}
!271 = !{!"float32", !272, i64 0}
!272 = !{!"0x32cecc0", !19, i64 0}
!273 = !{!274, !274, i64 0}
!274 = !{!"float32", !275, i64 0}
!275 = !{!"0x3319c50", !19, i64 0}
!276 = distinct !{!276, !27}
!277 = !{!278, !278, i64 0}
!278 = !{!"0x332e710.w1.b0", !279, i64 0}
!279 = !{!"0x332e710.w2.b0", !280, i64 0}
!280 = !{!"0x332e710.w4.b0", !281, i64 0}
!281 = !{!"0x332e710.w8.b0", !282, i64 0}
!282 = !{!"0x332e710.w16.b0", !283, i64 0}
!283 = !{!"0x332e710.w32.b0", !284, i64 0}
!284 = !{!"0x332e710.w64.b0", !285, i64 0}
!285 = !{!"0x332e710.w128.b0", !286, i64 0}
!286 = !{!"0x332e710.w256.b0", !287, i64 0}
!287 = !{!"0x332e710.w512.b0", !288, i64 0}
!288 = !{!"0x332e710.w1024.b0", !289, i64 0}
!289 = !{!"float32", !290, i64 0}
!290 = !{!"0x332e710", !19, i64 0}
!291 = !{!292, !292, i64 0}
!292 = !{!"float32", !293, i64 0}
!293 = !{!"0x332e6d0", !19, i64 0}
!294 = !{!295, !295, i64 0}
!295 = !{!"float32", !296, i64 0}
!296 = !{!"0x332eb50", !19, i64 0}
!297 = distinct !{!297, !27}
!298 = !{!299, !299, i64 0}
!299 = !{!"float32", !300, i64 0}
!300 = !{!"0x336e920", !19, i64 0}
!301 = !{!302, !302, i64 0}
!302 = !{!"float32", !303, i64 0}
!303 = !{!"0x336fa10", !19, i64 0}
!304 = !{!305, !305, i64 0}
!305 = !{!"float32", !306, i64 0}
!306 = !{!"0x333b6b0", !19, i64 0}
!307 = !{!308, !308, i64 0}
!308 = !{!"float32", !309, i64 0}
!309 = !{!"0x333b2b0", !19, i64 0}
!310 = distinct !{!310, !27}
!311 = !{!312, !312, i64 0}
!312 = !{!"float32", !313, i64 0}
!313 = !{!"0x33a6e40", !19, i64 0}
!314 = !{!315, !315, i64 0}
!315 = !{!"float32", !316, i64 0}
!316 = !{!"0x33a7220", !19, i64 0}
!317 = !{!318, !318, i64 0}
!318 = !{!"float32", !319, i64 0}
!319 = !{!"0x33a7330", !19, i64 0}
!320 = !{!321, !321, i64 0}
!321 = !{!"float32", !322, i64 0}
!322 = !{!"0x33a9e50", !19, i64 0}
!323 = !{!324, !324, i64 0}
!324 = !{!"0x33afc00.w8.b0", !325, i64 0}
!325 = !{!"0x33afc00.w16.b0", !326, i64 0}
!326 = !{!"0x33afc00.w32.b0", !327, i64 0}
!327 = !{!"0x33afc00.w64.b0", !328, i64 0}
!328 = !{!"0x33afc00.w128.b0", !329, i64 0}
!329 = !{!"0x33afc00.w256.b0", !330, i64 0}
!330 = !{!"0x33afc00.w512.b0", !331, i64 0}
!331 = !{!"0x33afc00.w1024.b0", !332, i64 0}
!332 = !{!"float32", !333, i64 0}
!333 = !{!"0x33afc00", !19, i64 0}
!334 = !{!332, !332, i64 0}
!335 = !{!336, !336, i64 0}
!336 = !{!"float32", !337, i64 0}
!337 = !{!"0x33b0200", !19, i64 0}
!338 = !{!339, !339, i64 0}
!339 = !{!"float32", !340, i64 0}
!340 = !{!"0x32d86a0", !19, i64 0}
!341 = !{!342, !342, i64 0}
!342 = !{!"float32", !343, i64 0}
!343 = !{!"0x32d86e0", !19, i64 0}
!344 = !{!345, !345, i64 0}
!345 = !{!"float32", !346, i64 0}
!346 = !{!"0x3341050", !19, i64 0}
!347 = distinct !{!347, !27}
!348 = !{!349, !349, i64 0}
!349 = !{!"float32", !350, i64 0}
!350 = !{!"0x32ec900", !19, i64 0}
!351 = !{!352, !352, i64 0}
!352 = !{!"float32", !353, i64 0}
!353 = !{!"0x32fab10", !19, i64 0}
!354 = distinct !{!354, !27}
!355 = !{!356, !356, i64 0}
!356 = !{!"float32", !357, i64 0}
!357 = !{!"0x32c64c0", !19, i64 0}
!358 = distinct !{!358, !27}
!359 = !{!360, !360, i64 0}
!360 = !{!"float32", !361, i64 0}
!361 = !{!"0x3324980", !19, i64 0}
!362 = !{!363, !363, i64 0}
!363 = !{!"float32", !364, i64 0}
!364 = !{!"0x3324fa0", !19, i64 0}
!365 = !{!366, !366, i64 0}
!366 = !{!"float32", !367, i64 0}
!367 = !{!"0x3324230", !19, i64 0}
!368 = !{!369, !369, i64 0}
!369 = !{!"float32", !370, i64 0}
!370 = !{!"0x3349600", !19, i64 0}
!371 = !{!372, !372, i64 0}
!372 = !{!"float32", !373, i64 0}
!373 = !{!"0x3349c20", !19, i64 0}
!374 = !{!375, !375, i64 0}
!375 = !{!"float32", !376, i64 0}
!376 = !{!"0x3348990", !19, i64 0}
!377 = !{!378, !378, i64 0}
!378 = !{!"0x3353640.w1.b0", !379, i64 0}
!379 = !{!"0x3353640.w2.b0", !380, i64 0}
!380 = !{!"0x3353640.w4.b0", !381, i64 0}
!381 = !{!"0x3353640.w8.b0", !382, i64 0}
!382 = !{!"0x3353640.w16.b0", !383, i64 0}
!383 = !{!"0x3353640.w32.b0", !384, i64 0}
!384 = !{!"0x3353640.w64.b0", !385, i64 0}
!385 = !{!"0x3353640.w128.b0", !386, i64 0}
!386 = !{!"0x3353640.w256.b0", !387, i64 0}
!387 = !{!"0x3353640.w512.b0", !388, i64 0}
!388 = !{!"0x3353640.w1024.b0", !389, i64 0}
!389 = !{!"float32", !390, i64 0}
!390 = !{!"0x3353640", !19, i64 0}
!391 = !{!392, !392, i64 0}
!392 = !{!"float32", !393, i64 0}
!393 = !{!"0x3353600", !19, i64 0}
!394 = !{!395, !395, i64 0}
!395 = !{!"float32", !396, i64 0}
!396 = !{!"0x3353920", !19, i64 0}
!397 = distinct !{!397, !27}
