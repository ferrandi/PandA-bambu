; ModuleID = 'fused_multiply_3'
source_filename = "fused_multiply_3"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%0 = type { i8*, %1, i32, %2, i64*, i64*, i64 }
%1 = type { i32, i32 }
%2 = type { i8, i8, i16 }

@__tvm_main__ = weak local_unnamed_addr constant [17 x i8] c"fused_multiply_3\00", align 1

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
define dllexport i32 @fused_nn_conv2d_4(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
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
  tail call fastcc void @fused_nn_conv2d_4_compute_(i8* %12, i8* %14, i8* %16)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_conv2d_4_compute_(i8* noalias nocapture readonly, i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #3 {
entry:
  %3 = alloca [27 x <8 x float>], align 32
  %4 = alloca [54 x <8 x float>], align 32
  %5 = alloca [4356 x <8 x float>], align 16
  %6 = alloca [149187 x float], align 4
  %.sub = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_begin1.preheader, %entry
  %indvar216 = phi i64 [ 0, %entry ], [ %indvar.next217, %for_begin1.preheader ]
  %7 = mul nuw nsw i64 %indvar216, 669
  %8 = mul nuw nsw i64 %indvar216, 896
  %scevgep220 = getelementptr [149187 x float], [149187 x float]* %6, i64 0, i64 %7
  %scevgep220221 = bitcast float* %scevgep220 to i8*
  %scevgep222 = getelementptr i8, i8* %0, i64 %8
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep220221, i8* align 4 %scevgep222, i64 892, i1 false)
  %9 = add nuw nsw i64 %7, 223
  %scevgep220.1 = getelementptr [149187 x float], [149187 x float]* %6, i64 0, i64 %9
  %scevgep220221.1 = bitcast float* %scevgep220.1 to i8*
  %10 = add nuw nsw i64 %8, 200704
  %scevgep222.1 = getelementptr i8, i8* %0, i64 %10
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep220221.1, i8* align 4 %scevgep222.1, i64 892, i1 false)
  %11 = add nuw nsw i64 %7, 446
  %scevgep220.2 = getelementptr [149187 x float], [149187 x float]* %6, i64 0, i64 %11
  %scevgep220221.2 = bitcast float* %scevgep220.2 to i8*
  %12 = add nuw nsw i64 %8, 401408
  %scevgep222.2 = getelementptr i8, i8* %0, i64 %12
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %scevgep220221.2, i8* align 4 %scevgep222.2, i64 892, i1 false)
  %indvar.next217 = add nuw nsw i64 %indvar216, 1
  %exitcond224 = icmp eq i64 %indvar.next217, 223
  br i1 %exitcond224, label %for_begin7.preheader, label %for_begin1.preheader, !prof !28

for_begin7.preheader:                             ; preds = %for_begin1.preheader
  %13 = bitcast [27 x <8 x float>]* %3 to i8*
  %14 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %indvars.iv210 = phi i64 [ 0, %for_begin7.preheader ], [ %indvars.iv.next211, %for_end12 ]
  %15 = mul nuw nsw i64 %indvars.iv210, 264
  %16 = trunc i64 %indvars.iv210 to i32
  %17 = urem i32 %16, 11
  %18 = mul nuw nsw i32 %17, 11
  %19 = udiv i32 %16, 11
  %20 = mul nsw i32 %19, 2904
  %21 = add nuw i32 %18, %20
  %22 = zext i32 %21 to i64
  br label %for_begin13.preheader

for_begin16.preheader:                            ; preds = %for_end12
  %23 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 8
  %24 = bitcast float* %23 to <8 x float>*
  %25 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 16
  %26 = bitcast float* %25 to <8 x float>*
  %27 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 24
  %28 = bitcast float* %27 to <8 x float>*
  %29 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 32
  %30 = bitcast float* %29 to <8 x float>*
  %31 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 40
  %32 = bitcast float* %31 to <8 x float>*
  %33 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 48
  %34 = bitcast float* %33 to <8 x float>*
  %35 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 56
  %36 = bitcast float* %35 to <8 x float>*
  %37 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 64
  %38 = bitcast float* %37 to <8 x float>*
  %39 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 72
  %40 = bitcast float* %39 to <8 x float>*
  %41 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 80
  %42 = bitcast float* %41 to <8 x float>*
  %43 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 88
  %44 = bitcast float* %43 to <8 x float>*
  %45 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 96
  %46 = bitcast float* %45 to <8 x float>*
  %47 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 104
  %48 = bitcast float* %47 to <8 x float>*
  %49 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 112
  %50 = bitcast float* %49 to <8 x float>*
  %51 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 120
  %52 = bitcast float* %51 to <8 x float>*
  %53 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 128
  %54 = bitcast float* %53 to <8 x float>*
  %55 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 136
  %56 = bitcast float* %55 to <8 x float>*
  %57 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 144
  %58 = bitcast float* %57 to <8 x float>*
  %59 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 152
  %60 = bitcast float* %59 to <8 x float>*
  %61 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 160
  %62 = bitcast float* %61 to <8 x float>*
  %63 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 168
  %64 = bitcast float* %63 to <8 x float>*
  %65 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 176
  %66 = bitcast float* %65 to <8 x float>*
  %67 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 184
  %68 = bitcast float* %67 to <8 x float>*
  %69 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 192
  %70 = bitcast float* %69 to <8 x float>*
  %71 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 200
  %72 = bitcast float* %71 to <8 x float>*
  %73 = getelementptr inbounds [27 x <8 x float>], [27 x <8 x float>]* %3, i64 0, i64 0, i64 208
  %74 = bitcast float* %73 to <8 x float>*
  %75 = bitcast i8* %2 to float*
  %76 = bitcast [27 x <8 x float>]* %3 to i8*
  br label %for_begin19.preheader

for_begin13.preheader:                            ; preds = %for_begin13.preheader, %for_begin10.preheader
  %indvars.iv207 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next208, %for_begin13.preheader ]
  %77 = mul nuw nsw i64 %indvars.iv207, 24
  %78 = add nuw nsw i64 %77, %15
  %79 = add nuw nsw i64 %indvars.iv207, %22
  %80 = add nuw nsw i64 %79, 363
  %81 = add nuw nsw i64 %79, 726
  %82 = add nuw nsw i64 %79, 1089
  %83 = add nuw nsw i64 %79, 1452
  %84 = add nuw nsw i64 %79, 1815
  %85 = add nuw nsw i64 %79, 2178
  %86 = add nuw nsw i64 %79, 2541
  %87 = getelementptr inbounds float, float* %14, i64 %79
  %88 = load float, float* %87, align 4, !tbaa !29
  %89 = insertelement <8 x float> undef, float %88, i32 0
  %90 = getelementptr inbounds float, float* %14, i64 %80
  %91 = load float, float* %90, align 4, !tbaa !29
  %92 = insertelement <8 x float> %89, float %91, i32 1
  %93 = getelementptr inbounds float, float* %14, i64 %81
  %94 = load float, float* %93, align 4, !tbaa !29
  %95 = insertelement <8 x float> %92, float %94, i32 2
  %96 = getelementptr inbounds float, float* %14, i64 %82
  %97 = load float, float* %96, align 4, !tbaa !29
  %98 = insertelement <8 x float> %95, float %97, i32 3
  %99 = getelementptr inbounds float, float* %14, i64 %83
  %100 = load float, float* %99, align 4, !tbaa !29
  %101 = insertelement <8 x float> %98, float %100, i32 4
  %102 = getelementptr inbounds float, float* %14, i64 %84
  %103 = load float, float* %102, align 4, !tbaa !29
  %104 = insertelement <8 x float> %101, float %103, i32 5
  %105 = getelementptr inbounds float, float* %14, i64 %85
  %106 = load float, float* %105, align 4, !tbaa !29
  %107 = insertelement <8 x float> %104, float %106, i32 6
  %108 = getelementptr inbounds float, float* %14, i64 %86
  %109 = load float, float* %108, align 4, !tbaa !29
  %110 = insertelement <8 x float> %107, float %109, i32 7
  %111 = getelementptr inbounds [4356 x <8 x float>], [4356 x <8 x float>]* %5, i64 0, i64 0, i64 %78
  %112 = bitcast float* %111 to <8 x float>*
  store <8 x float> %110, <8 x float>* %112, align 16, !tbaa !32
  %113 = add nuw nsw i64 %78, 8
  %114 = add nuw nsw i64 %79, 121
  %115 = add nuw nsw i64 %79, 484
  %116 = add nuw nsw i64 %79, 847
  %117 = add nuw nsw i64 %79, 1210
  %118 = add nuw nsw i64 %79, 1573
  %119 = add nuw nsw i64 %79, 1936
  %120 = add nuw nsw i64 %79, 2299
  %121 = add nuw nsw i64 %79, 2662
  %122 = getelementptr inbounds float, float* %14, i64 %114
  %123 = load float, float* %122, align 4, !tbaa !29
  %124 = insertelement <8 x float> undef, float %123, i32 0
  %125 = getelementptr inbounds float, float* %14, i64 %115
  %126 = load float, float* %125, align 4, !tbaa !29
  %127 = insertelement <8 x float> %124, float %126, i32 1
  %128 = getelementptr inbounds float, float* %14, i64 %116
  %129 = load float, float* %128, align 4, !tbaa !29
  %130 = insertelement <8 x float> %127, float %129, i32 2
  %131 = getelementptr inbounds float, float* %14, i64 %117
  %132 = load float, float* %131, align 4, !tbaa !29
  %133 = insertelement <8 x float> %130, float %132, i32 3
  %134 = getelementptr inbounds float, float* %14, i64 %118
  %135 = load float, float* %134, align 4, !tbaa !29
  %136 = insertelement <8 x float> %133, float %135, i32 4
  %137 = getelementptr inbounds float, float* %14, i64 %119
  %138 = load float, float* %137, align 4, !tbaa !29
  %139 = insertelement <8 x float> %136, float %138, i32 5
  %140 = getelementptr inbounds float, float* %14, i64 %120
  %141 = load float, float* %140, align 4, !tbaa !29
  %142 = insertelement <8 x float> %139, float %141, i32 6
  %143 = getelementptr inbounds float, float* %14, i64 %121
  %144 = load float, float* %143, align 4, !tbaa !29
  %145 = insertelement <8 x float> %142, float %144, i32 7
  %146 = getelementptr inbounds [4356 x <8 x float>], [4356 x <8 x float>]* %5, i64 0, i64 0, i64 %113
  %147 = bitcast float* %146 to <8 x float>*
  store <8 x float> %145, <8 x float>* %147, align 16, !tbaa !32
  %148 = add nuw nsw i64 %78, 16
  %149 = add nuw nsw i64 %79, 242
  %150 = add nuw nsw i64 %79, 605
  %151 = add nuw nsw i64 %79, 968
  %152 = add nuw nsw i64 %79, 1331
  %153 = add nuw nsw i64 %79, 1694
  %154 = add nuw nsw i64 %79, 2057
  %155 = add nuw nsw i64 %79, 2420
  %156 = add nuw nsw i64 %79, 2783
  %157 = getelementptr inbounds float, float* %14, i64 %149
  %158 = load float, float* %157, align 4, !tbaa !29
  %159 = insertelement <8 x float> undef, float %158, i32 0
  %160 = getelementptr inbounds float, float* %14, i64 %150
  %161 = load float, float* %160, align 4, !tbaa !29
  %162 = insertelement <8 x float> %159, float %161, i32 1
  %163 = getelementptr inbounds float, float* %14, i64 %151
  %164 = load float, float* %163, align 4, !tbaa !29
  %165 = insertelement <8 x float> %162, float %164, i32 2
  %166 = getelementptr inbounds float, float* %14, i64 %152
  %167 = load float, float* %166, align 4, !tbaa !29
  %168 = insertelement <8 x float> %165, float %167, i32 3
  %169 = getelementptr inbounds float, float* %14, i64 %153
  %170 = load float, float* %169, align 4, !tbaa !29
  %171 = insertelement <8 x float> %168, float %170, i32 4
  %172 = getelementptr inbounds float, float* %14, i64 %154
  %173 = load float, float* %172, align 4, !tbaa !29
  %174 = insertelement <8 x float> %171, float %173, i32 5
  %175 = getelementptr inbounds float, float* %14, i64 %155
  %176 = load float, float* %175, align 4, !tbaa !29
  %177 = insertelement <8 x float> %174, float %176, i32 6
  %178 = getelementptr inbounds float, float* %14, i64 %156
  %179 = load float, float* %178, align 4, !tbaa !29
  %180 = insertelement <8 x float> %177, float %179, i32 7
  %181 = getelementptr inbounds [4356 x <8 x float>], [4356 x <8 x float>]* %5, i64 0, i64 0, i64 %148
  %182 = bitcast float* %181 to <8 x float>*
  store <8 x float> %180, <8 x float>* %182, align 16, !tbaa !32
  %indvars.iv.next208 = add nuw nsw i64 %indvars.iv207, 1
  %exitcond209 = icmp eq i64 %indvars.iv.next208, 11
  br i1 %exitcond209, label %for_end12, label %for_begin13.preheader, !prof !28

for_end12:                                        ; preds = %for_begin13.preheader
  %indvars.iv.next211 = add nuw nsw i64 %indvars.iv210, 1
  %exitcond212 = icmp eq i64 %indvars.iv.next211, 132
  br i1 %exitcond212, label %for_begin16.preheader, label %for_begin10.preheader, !prof !28

for_begin19.preheader:                            ; preds = %for_end39.1, %for_begin16.preheader
  %183 = phi i32 [ 0, %for_begin16.preheader ], [ %458, %for_end39.1 ]
  %184 = urem i32 %183, 54
  %185 = mul nuw nsw i32 %184, 2676
  %186 = udiv i32 %183, 54
  %187 = mul nsw i32 %186, 2904
  %188 = zext i32 %187 to i64
  %189 = zext i32 %185 to i64
  br label %for_body20

for_end18:                                        ; preds = %for_end39.1
  ret void

for_begin34.preheader:                            ; preds = %for_begin31.preheader
  %190 = mul nuw nsw i32 %184, 54
  %191 = mul nsw i32 %186, 23328
  %192 = add nuw nsw i32 %191, %190
  %193 = zext i32 %192 to i64
  br label %for_body38

for_body20:                                       ; preds = %for_begin31.preheader, %for_begin19.preheader
  %indvar = phi i64 [ 0, %for_begin19.preheader ], [ %indvar.next, %for_begin31.preheader ]
  %194 = mul nuw nsw i64 %indvar, 27
  %scevgep = getelementptr [54 x <8 x float>], [54 x <8 x float>]* %4, i64 0, i64 %194
  %scevgep195 = bitcast <8 x float>* %scevgep to i8*
  %195 = mul nuw nsw i64 %indvar, 108
  %196 = add nuw nsw i64 %195, %189
  call void @llvm.memset.p0i8.i64(i8* nonnull align 32 %76, i8 0, i64 864, i1 false)
  br label %for_begin25.preheader

for_begin31.preheader:                            ; preds = %for_end27
  store <8 x float> %243, <8 x float>* %.sub, align 32, !tbaa !35
  store <8 x float> %249, <8 x float>* %24, align 32, !tbaa !35
  store <8 x float> %255, <8 x float>* %26, align 32, !tbaa !35
  store <8 x float> %261, <8 x float>* %28, align 32, !tbaa !35
  store <8 x float> %267, <8 x float>* %30, align 32, !tbaa !35
  store <8 x float> %273, <8 x float>* %32, align 32, !tbaa !35
  store <8 x float> %279, <8 x float>* %34, align 32, !tbaa !35
  store <8 x float> %285, <8 x float>* %36, align 32, !tbaa !35
  store <8 x float> %291, <8 x float>* %38, align 32, !tbaa !35
  store <8 x float> %297, <8 x float>* %40, align 32, !tbaa !35
  store <8 x float> %303, <8 x float>* %42, align 32, !tbaa !35
  store <8 x float> %309, <8 x float>* %44, align 32, !tbaa !35
  store <8 x float> %315, <8 x float>* %46, align 32, !tbaa !35
  store <8 x float> %321, <8 x float>* %48, align 32, !tbaa !35
  store <8 x float> %327, <8 x float>* %50, align 32, !tbaa !35
  store <8 x float> %333, <8 x float>* %52, align 32, !tbaa !35
  store <8 x float> %339, <8 x float>* %54, align 32, !tbaa !35
  store <8 x float> %345, <8 x float>* %56, align 32, !tbaa !35
  store <8 x float> %351, <8 x float>* %58, align 32, !tbaa !35
  store <8 x float> %357, <8 x float>* %60, align 32, !tbaa !35
  store <8 x float> %363, <8 x float>* %62, align 32, !tbaa !35
  store <8 x float> %369, <8 x float>* %64, align 32, !tbaa !35
  store <8 x float> %375, <8 x float>* %66, align 32, !tbaa !35
  store <8 x float> %381, <8 x float>* %68, align 32, !tbaa !35
  store <8 x float> %387, <8 x float>* %70, align 32, !tbaa !35
  store <8 x float> %393, <8 x float>* %72, align 32, !tbaa !35
  store <8 x float> %399, <8 x float>* %74, align 32, !tbaa !35
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 32 %scevgep195, i8* nonnull align 32 %13, i64 864, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond196 = icmp eq i64 %indvar.next, 2
  br i1 %exitcond196, label %for_begin34.preheader, label %for_body20, !prof !28

for_begin25.preheader:                            ; preds = %for_end27, %for_body20
  %indvars.iv189 = phi i64 [ 0, %for_body20 ], [ %indvars.iv.next190, %for_end27 ]
  %.lcssa54.lcssa159 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %399, %for_end27 ]
  %.lcssa52.lcssa157 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %393, %for_end27 ]
  %.lcssa50.lcssa155 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %387, %for_end27 ]
  %.lcssa48.lcssa153 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %381, %for_end27 ]
  %.lcssa46.lcssa151 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %375, %for_end27 ]
  %.lcssa44.lcssa149 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %369, %for_end27 ]
  %.lcssa42.lcssa147 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %363, %for_end27 ]
  %.lcssa40.lcssa145 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %357, %for_end27 ]
  %.lcssa38.lcssa143 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %351, %for_end27 ]
  %.lcssa36.lcssa141 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %345, %for_end27 ]
  %.lcssa34.lcssa139 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %339, %for_end27 ]
  %.lcssa32.lcssa137 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %333, %for_end27 ]
  %.lcssa30.lcssa135 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %327, %for_end27 ]
  %.lcssa28.lcssa133 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %321, %for_end27 ]
  %.lcssa26.lcssa131 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %315, %for_end27 ]
  %.lcssa24.lcssa129 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %309, %for_end27 ]
  %.lcssa22.lcssa127 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %303, %for_end27 ]
  %.lcssa20.lcssa125 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %297, %for_end27 ]
  %.lcssa18.lcssa123 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %291, %for_end27 ]
  %.lcssa16.lcssa121 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %285, %for_end27 ]
  %.lcssa14.lcssa119 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %279, %for_end27 ]
  %.lcssa12.lcssa117 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %273, %for_end27 ]
  %.lcssa10.lcssa115 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %267, %for_end27 ]
  %.lcssa8.lcssa113 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %261, %for_end27 ]
  %.lcssa6.lcssa112 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %255, %for_end27 ]
  %.lcssa4.lcssa110 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %249, %for_end27 ]
  %.lcssa.lcssa108 = phi <8 x float> [ zeroinitializer, %for_body20 ], [ %243, %for_end27 ]
  %197 = mul nuw nsw i64 %indvars.iv189, 669
  %198 = add nuw nsw i64 %196, %197
  %199 = mul nuw nsw i64 %indvars.iv189, 264
  %200 = add nuw nsw i64 %199, %188
  br label %for_begin28.preheader

for_begin28.preheader:                            ; preds = %for_end30, %for_begin25.preheader
  %indvars.iv186 = phi i64 [ 0, %for_begin25.preheader ], [ %indvars.iv.next187, %for_end30 ]
  %.lcssa54107 = phi <8 x float> [ %.lcssa54.lcssa159, %for_begin25.preheader ], [ %399, %for_end30 ]
  %.lcssa52105 = phi <8 x float> [ %.lcssa52.lcssa157, %for_begin25.preheader ], [ %393, %for_end30 ]
  %.lcssa50103 = phi <8 x float> [ %.lcssa50.lcssa155, %for_begin25.preheader ], [ %387, %for_end30 ]
  %.lcssa48101 = phi <8 x float> [ %.lcssa48.lcssa153, %for_begin25.preheader ], [ %381, %for_end30 ]
  %.lcssa4699 = phi <8 x float> [ %.lcssa46.lcssa151, %for_begin25.preheader ], [ %375, %for_end30 ]
  %.lcssa4497 = phi <8 x float> [ %.lcssa44.lcssa149, %for_begin25.preheader ], [ %369, %for_end30 ]
  %.lcssa4295 = phi <8 x float> [ %.lcssa42.lcssa147, %for_begin25.preheader ], [ %363, %for_end30 ]
  %.lcssa4093 = phi <8 x float> [ %.lcssa40.lcssa145, %for_begin25.preheader ], [ %357, %for_end30 ]
  %.lcssa3891 = phi <8 x float> [ %.lcssa38.lcssa143, %for_begin25.preheader ], [ %351, %for_end30 ]
  %.lcssa3689 = phi <8 x float> [ %.lcssa36.lcssa141, %for_begin25.preheader ], [ %345, %for_end30 ]
  %.lcssa3487 = phi <8 x float> [ %.lcssa34.lcssa139, %for_begin25.preheader ], [ %339, %for_end30 ]
  %.lcssa3285 = phi <8 x float> [ %.lcssa32.lcssa137, %for_begin25.preheader ], [ %333, %for_end30 ]
  %.lcssa3083 = phi <8 x float> [ %.lcssa30.lcssa135, %for_begin25.preheader ], [ %327, %for_end30 ]
  %.lcssa2881 = phi <8 x float> [ %.lcssa28.lcssa133, %for_begin25.preheader ], [ %321, %for_end30 ]
  %.lcssa2679 = phi <8 x float> [ %.lcssa26.lcssa131, %for_begin25.preheader ], [ %315, %for_end30 ]
  %.lcssa2477 = phi <8 x float> [ %.lcssa24.lcssa129, %for_begin25.preheader ], [ %309, %for_end30 ]
  %.lcssa2275 = phi <8 x float> [ %.lcssa22.lcssa127, %for_begin25.preheader ], [ %303, %for_end30 ]
  %.lcssa2073 = phi <8 x float> [ %.lcssa20.lcssa125, %for_begin25.preheader ], [ %297, %for_end30 ]
  %.lcssa1871 = phi <8 x float> [ %.lcssa18.lcssa123, %for_begin25.preheader ], [ %291, %for_end30 ]
  %.lcssa1669 = phi <8 x float> [ %.lcssa16.lcssa121, %for_begin25.preheader ], [ %285, %for_end30 ]
  %.lcssa1467 = phi <8 x float> [ %.lcssa14.lcssa119, %for_begin25.preheader ], [ %279, %for_end30 ]
  %.lcssa1265 = phi <8 x float> [ %.lcssa12.lcssa117, %for_begin25.preheader ], [ %273, %for_end30 ]
  %.lcssa1063 = phi <8 x float> [ %.lcssa10.lcssa115, %for_begin25.preheader ], [ %267, %for_end30 ]
  %.lcssa861 = phi <8 x float> [ %.lcssa8.lcssa113, %for_begin25.preheader ], [ %261, %for_end30 ]
  %.lcssa659 = phi <8 x float> [ %.lcssa6.lcssa112, %for_begin25.preheader ], [ %255, %for_end30 ]
  %.lcssa458 = phi <8 x float> [ %.lcssa4.lcssa110, %for_begin25.preheader ], [ %249, %for_end30 ]
  %.lcssa56 = phi <8 x float> [ %.lcssa.lcssa108, %for_begin25.preheader ], [ %243, %for_end30 ]
  %201 = add nuw nsw i64 %198, %indvars.iv186
  %202 = mul nuw nsw i64 %indvars.iv186, 24
  %203 = add nuw nsw i64 %200, %202
  br label %for_body29

for_end27:                                        ; preds = %for_end30
  %indvars.iv.next190 = add nuw nsw i64 %indvars.iv189, 1
  %exitcond191 = icmp eq i64 %indvars.iv.next190, 11
  br i1 %exitcond191, label %for_begin31.preheader, label %for_begin25.preheader, !prof !28

for_body29:                                       ; preds = %for_body29, %for_begin28.preheader
  %indvars.iv = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next, %for_body29 ]
  %204 = phi <8 x float> [ %.lcssa54107, %for_begin28.preheader ], [ %399, %for_body29 ]
  %205 = phi <8 x float> [ %.lcssa52105, %for_begin28.preheader ], [ %393, %for_body29 ]
  %206 = phi <8 x float> [ %.lcssa50103, %for_begin28.preheader ], [ %387, %for_body29 ]
  %207 = phi <8 x float> [ %.lcssa48101, %for_begin28.preheader ], [ %381, %for_body29 ]
  %208 = phi <8 x float> [ %.lcssa4699, %for_begin28.preheader ], [ %375, %for_body29 ]
  %209 = phi <8 x float> [ %.lcssa4497, %for_begin28.preheader ], [ %369, %for_body29 ]
  %210 = phi <8 x float> [ %.lcssa4295, %for_begin28.preheader ], [ %363, %for_body29 ]
  %211 = phi <8 x float> [ %.lcssa4093, %for_begin28.preheader ], [ %357, %for_body29 ]
  %212 = phi <8 x float> [ %.lcssa3891, %for_begin28.preheader ], [ %351, %for_body29 ]
  %213 = phi <8 x float> [ %.lcssa3689, %for_begin28.preheader ], [ %345, %for_body29 ]
  %214 = phi <8 x float> [ %.lcssa3487, %for_begin28.preheader ], [ %339, %for_body29 ]
  %215 = phi <8 x float> [ %.lcssa3285, %for_begin28.preheader ], [ %333, %for_body29 ]
  %216 = phi <8 x float> [ %.lcssa3083, %for_begin28.preheader ], [ %327, %for_body29 ]
  %217 = phi <8 x float> [ %.lcssa2881, %for_begin28.preheader ], [ %321, %for_body29 ]
  %218 = phi <8 x float> [ %.lcssa2679, %for_begin28.preheader ], [ %315, %for_body29 ]
  %219 = phi <8 x float> [ %.lcssa2477, %for_begin28.preheader ], [ %309, %for_body29 ]
  %220 = phi <8 x float> [ %.lcssa2275, %for_begin28.preheader ], [ %303, %for_body29 ]
  %221 = phi <8 x float> [ %.lcssa2073, %for_begin28.preheader ], [ %297, %for_body29 ]
  %222 = phi <8 x float> [ %.lcssa1871, %for_begin28.preheader ], [ %291, %for_body29 ]
  %223 = phi <8 x float> [ %.lcssa1669, %for_begin28.preheader ], [ %285, %for_body29 ]
  %224 = phi <8 x float> [ %.lcssa1467, %for_begin28.preheader ], [ %279, %for_body29 ]
  %225 = phi <8 x float> [ %.lcssa1265, %for_begin28.preheader ], [ %273, %for_body29 ]
  %226 = phi <8 x float> [ %.lcssa1063, %for_begin28.preheader ], [ %267, %for_body29 ]
  %227 = phi <8 x float> [ %.lcssa861, %for_begin28.preheader ], [ %261, %for_body29 ]
  %228 = phi <8 x float> [ %.lcssa659, %for_begin28.preheader ], [ %255, %for_body29 ]
  %229 = phi <8 x float> [ %.lcssa458, %for_begin28.preheader ], [ %249, %for_body29 ]
  %230 = phi <8 x float> [ %.lcssa56, %for_begin28.preheader ], [ %243, %for_body29 ]
  %231 = mul nuw nsw i64 %indvars.iv, 223
  %232 = add nuw nsw i64 %201, %231
  %233 = and i64 %232, 4294967295
  %234 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %233
  %235 = load float, float* %234, align 4, !tbaa !46
  %236 = insertelement <8 x float> undef, float %235, i32 0
  %237 = shufflevector <8 x float> %236, <8 x float> undef, <8 x i32> zeroinitializer
  %238 = shl i64 %indvars.iv, 3
  %239 = add nuw nsw i64 %203, %238
  %240 = getelementptr inbounds [4356 x <8 x float>], [4356 x <8 x float>]* %5, i64 0, i64 0, i64 %239
  %241 = bitcast float* %240 to <8 x float>*
  %242 = load <8 x float>, <8 x float>* %241, align 16, !tbaa !32
  %243 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %237, <8 x float> %242, <8 x float> %230)
  %244 = add nuw nsw i64 %232, 4
  %245 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %244
  %246 = load float, float* %245, align 4, !tbaa !46
  %247 = insertelement <8 x float> undef, float %246, i32 0
  %248 = shufflevector <8 x float> %247, <8 x float> undef, <8 x i32> zeroinitializer
  %249 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %248, <8 x float> %242, <8 x float> %229)
  %250 = add nuw nsw i64 %232, 8
  %251 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %250
  %252 = load float, float* %251, align 4, !tbaa !46
  %253 = insertelement <8 x float> undef, float %252, i32 0
  %254 = shufflevector <8 x float> %253, <8 x float> undef, <8 x i32> zeroinitializer
  %255 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %254, <8 x float> %242, <8 x float> %228)
  %256 = add nuw nsw i64 %232, 12
  %257 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %256
  %258 = load float, float* %257, align 4, !tbaa !46
  %259 = insertelement <8 x float> undef, float %258, i32 0
  %260 = shufflevector <8 x float> %259, <8 x float> undef, <8 x i32> zeroinitializer
  %261 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %260, <8 x float> %242, <8 x float> %227)
  %262 = add nuw nsw i64 %232, 16
  %263 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %262
  %264 = load float, float* %263, align 4, !tbaa !46
  %265 = insertelement <8 x float> undef, float %264, i32 0
  %266 = shufflevector <8 x float> %265, <8 x float> undef, <8 x i32> zeroinitializer
  %267 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %266, <8 x float> %242, <8 x float> %226)
  %268 = add nuw nsw i64 %232, 20
  %269 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %268
  %270 = load float, float* %269, align 4, !tbaa !46
  %271 = insertelement <8 x float> undef, float %270, i32 0
  %272 = shufflevector <8 x float> %271, <8 x float> undef, <8 x i32> zeroinitializer
  %273 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %272, <8 x float> %242, <8 x float> %225)
  %274 = add nuw nsw i64 %232, 24
  %275 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %274
  %276 = load float, float* %275, align 4, !tbaa !46
  %277 = insertelement <8 x float> undef, float %276, i32 0
  %278 = shufflevector <8 x float> %277, <8 x float> undef, <8 x i32> zeroinitializer
  %279 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %278, <8 x float> %242, <8 x float> %224)
  %280 = add nuw nsw i64 %232, 28
  %281 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %280
  %282 = load float, float* %281, align 4, !tbaa !46
  %283 = insertelement <8 x float> undef, float %282, i32 0
  %284 = shufflevector <8 x float> %283, <8 x float> undef, <8 x i32> zeroinitializer
  %285 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %284, <8 x float> %242, <8 x float> %223)
  %286 = add nuw nsw i64 %232, 32
  %287 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %286
  %288 = load float, float* %287, align 4, !tbaa !46
  %289 = insertelement <8 x float> undef, float %288, i32 0
  %290 = shufflevector <8 x float> %289, <8 x float> undef, <8 x i32> zeroinitializer
  %291 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %290, <8 x float> %242, <8 x float> %222)
  %292 = add nuw nsw i64 %232, 36
  %293 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %292
  %294 = load float, float* %293, align 4, !tbaa !46
  %295 = insertelement <8 x float> undef, float %294, i32 0
  %296 = shufflevector <8 x float> %295, <8 x float> undef, <8 x i32> zeroinitializer
  %297 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %296, <8 x float> %242, <8 x float> %221)
  %298 = add nuw nsw i64 %232, 40
  %299 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %298
  %300 = load float, float* %299, align 4, !tbaa !46
  %301 = insertelement <8 x float> undef, float %300, i32 0
  %302 = shufflevector <8 x float> %301, <8 x float> undef, <8 x i32> zeroinitializer
  %303 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %302, <8 x float> %242, <8 x float> %220)
  %304 = add nuw nsw i64 %232, 44
  %305 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %304
  %306 = load float, float* %305, align 4, !tbaa !46
  %307 = insertelement <8 x float> undef, float %306, i32 0
  %308 = shufflevector <8 x float> %307, <8 x float> undef, <8 x i32> zeroinitializer
  %309 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %308, <8 x float> %242, <8 x float> %219)
  %310 = add nuw nsw i64 %232, 48
  %311 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %310
  %312 = load float, float* %311, align 4, !tbaa !46
  %313 = insertelement <8 x float> undef, float %312, i32 0
  %314 = shufflevector <8 x float> %313, <8 x float> undef, <8 x i32> zeroinitializer
  %315 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %314, <8 x float> %242, <8 x float> %218)
  %316 = add nuw nsw i64 %232, 52
  %317 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %316
  %318 = load float, float* %317, align 4, !tbaa !46
  %319 = insertelement <8 x float> undef, float %318, i32 0
  %320 = shufflevector <8 x float> %319, <8 x float> undef, <8 x i32> zeroinitializer
  %321 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %320, <8 x float> %242, <8 x float> %217)
  %322 = add nuw nsw i64 %232, 56
  %323 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %322
  %324 = load float, float* %323, align 4, !tbaa !46
  %325 = insertelement <8 x float> undef, float %324, i32 0
  %326 = shufflevector <8 x float> %325, <8 x float> undef, <8 x i32> zeroinitializer
  %327 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %326, <8 x float> %242, <8 x float> %216)
  %328 = add nuw nsw i64 %232, 60
  %329 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %328
  %330 = load float, float* %329, align 4, !tbaa !46
  %331 = insertelement <8 x float> undef, float %330, i32 0
  %332 = shufflevector <8 x float> %331, <8 x float> undef, <8 x i32> zeroinitializer
  %333 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %332, <8 x float> %242, <8 x float> %215)
  %334 = add nuw nsw i64 %232, 64
  %335 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %334
  %336 = load float, float* %335, align 4, !tbaa !46
  %337 = insertelement <8 x float> undef, float %336, i32 0
  %338 = shufflevector <8 x float> %337, <8 x float> undef, <8 x i32> zeroinitializer
  %339 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %338, <8 x float> %242, <8 x float> %214)
  %340 = add nuw nsw i64 %232, 68
  %341 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %340
  %342 = load float, float* %341, align 4, !tbaa !46
  %343 = insertelement <8 x float> undef, float %342, i32 0
  %344 = shufflevector <8 x float> %343, <8 x float> undef, <8 x i32> zeroinitializer
  %345 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %344, <8 x float> %242, <8 x float> %213)
  %346 = add nuw nsw i64 %232, 72
  %347 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %346
  %348 = load float, float* %347, align 4, !tbaa !46
  %349 = insertelement <8 x float> undef, float %348, i32 0
  %350 = shufflevector <8 x float> %349, <8 x float> undef, <8 x i32> zeroinitializer
  %351 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %350, <8 x float> %242, <8 x float> %212)
  %352 = add nuw nsw i64 %232, 76
  %353 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %352
  %354 = load float, float* %353, align 4, !tbaa !46
  %355 = insertelement <8 x float> undef, float %354, i32 0
  %356 = shufflevector <8 x float> %355, <8 x float> undef, <8 x i32> zeroinitializer
  %357 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %356, <8 x float> %242, <8 x float> %211)
  %358 = add nuw nsw i64 %232, 80
  %359 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %358
  %360 = load float, float* %359, align 4, !tbaa !46
  %361 = insertelement <8 x float> undef, float %360, i32 0
  %362 = shufflevector <8 x float> %361, <8 x float> undef, <8 x i32> zeroinitializer
  %363 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %362, <8 x float> %242, <8 x float> %210)
  %364 = add nuw nsw i64 %232, 84
  %365 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %364
  %366 = load float, float* %365, align 4, !tbaa !46
  %367 = insertelement <8 x float> undef, float %366, i32 0
  %368 = shufflevector <8 x float> %367, <8 x float> undef, <8 x i32> zeroinitializer
  %369 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %368, <8 x float> %242, <8 x float> %209)
  %370 = add nuw nsw i64 %232, 88
  %371 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %370
  %372 = load float, float* %371, align 4, !tbaa !46
  %373 = insertelement <8 x float> undef, float %372, i32 0
  %374 = shufflevector <8 x float> %373, <8 x float> undef, <8 x i32> zeroinitializer
  %375 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %374, <8 x float> %242, <8 x float> %208)
  %376 = add nuw nsw i64 %232, 92
  %377 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %376
  %378 = load float, float* %377, align 4, !tbaa !46
  %379 = insertelement <8 x float> undef, float %378, i32 0
  %380 = shufflevector <8 x float> %379, <8 x float> undef, <8 x i32> zeroinitializer
  %381 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %380, <8 x float> %242, <8 x float> %207)
  %382 = add nuw nsw i64 %232, 96
  %383 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %382
  %384 = load float, float* %383, align 4, !tbaa !46
  %385 = insertelement <8 x float> undef, float %384, i32 0
  %386 = shufflevector <8 x float> %385, <8 x float> undef, <8 x i32> zeroinitializer
  %387 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %386, <8 x float> %242, <8 x float> %206)
  %388 = add nuw nsw i64 %232, 100
  %389 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %388
  %390 = load float, float* %389, align 4, !tbaa !46
  %391 = insertelement <8 x float> undef, float %390, i32 0
  %392 = shufflevector <8 x float> %391, <8 x float> undef, <8 x i32> zeroinitializer
  %393 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %392, <8 x float> %242, <8 x float> %205)
  %394 = add nuw nsw i64 %232, 104
  %395 = getelementptr inbounds [149187 x float], [149187 x float]* %6, i64 0, i64 %394
  %396 = load float, float* %395, align 4, !tbaa !46
  %397 = insertelement <8 x float> undef, float %396, i32 0
  %398 = shufflevector <8 x float> %397, <8 x float> undef, <8 x i32> zeroinitializer
  %399 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %398, <8 x float> %242, <8 x float> %204)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 3
  br i1 %exitcond, label %for_end30, label %for_body29, !prof !28

for_end30:                                        ; preds = %for_body29
  %indvars.iv.next187 = add nuw nsw i64 %indvars.iv186, 1
  %exitcond188 = icmp eq i64 %indvars.iv.next187, 11
  br i1 %exitcond188, label %for_end27, label %for_begin28.preheader, !prof !28

for_body38:                                       ; preds = %for_body38, %for_begin34.preheader
  %indvars.iv197 = phi i64 [ 0, %for_begin34.preheader ], [ %indvars.iv.next198, %for_body38 ]
  %400 = add nuw nsw i64 %indvars.iv197, %193
  %401 = add nuw nsw i64 %400, 2916
  %402 = add nuw nsw i64 %400, 5832
  %403 = add nuw nsw i64 %400, 8748
  %404 = add nuw nsw i64 %400, 11664
  %405 = add nuw nsw i64 %400, 14580
  %406 = add nuw nsw i64 %400, 17496
  %407 = add nuw nsw i64 %400, 20412
  %408 = shl i64 %indvars.iv197, 3
  %409 = getelementptr inbounds [54 x <8 x float>], [54 x <8 x float>]* %4, i64 0, i64 0, i64 %408
  %410 = bitcast float* %409 to <8 x float>*
  %411 = load <8 x float>, <8 x float>* %410, align 32, !tbaa !49
  %412 = getelementptr inbounds float, float* %75, i64 %400
  %413 = extractelement <8 x float> %411, i64 0
  store float %413, float* %412, align 4, !tbaa !52
  %414 = getelementptr inbounds float, float* %75, i64 %401
  %415 = extractelement <8 x float> %411, i64 1
  store float %415, float* %414, align 4, !tbaa !52
  %416 = getelementptr inbounds float, float* %75, i64 %402
  %417 = extractelement <8 x float> %411, i64 2
  store float %417, float* %416, align 4, !tbaa !52
  %418 = getelementptr inbounds float, float* %75, i64 %403
  %419 = extractelement <8 x float> %411, i64 3
  store float %419, float* %418, align 4, !tbaa !52
  %420 = getelementptr inbounds float, float* %75, i64 %404
  %421 = extractelement <8 x float> %411, i64 4
  store float %421, float* %420, align 4, !tbaa !52
  %422 = getelementptr inbounds float, float* %75, i64 %405
  %423 = extractelement <8 x float> %411, i64 5
  store float %423, float* %422, align 4, !tbaa !52
  %424 = getelementptr inbounds float, float* %75, i64 %406
  %425 = extractelement <8 x float> %411, i64 6
  store float %425, float* %424, align 4, !tbaa !52
  %426 = getelementptr inbounds float, float* %75, i64 %407
  %427 = extractelement <8 x float> %411, i64 7
  store float %427, float* %426, align 4, !tbaa !52
  %indvars.iv.next198 = add nuw nsw i64 %indvars.iv197, 1
  %exitcond199 = icmp eq i64 %indvars.iv.next198, 27
  br i1 %exitcond199, label %for_end39, label %for_body38, !prof !28

for_end39:                                        ; preds = %for_body38
  %428 = add nuw nsw i64 %193, 27
  br label %for_body38.1

for_body38.1:                                     ; preds = %for_body38.1, %for_end39
  %indvars.iv197.1 = phi i64 [ 0, %for_end39 ], [ %indvars.iv.next198.1, %for_body38.1 ]
  %429 = add nuw nsw i64 %428, %indvars.iv197.1
  %430 = add nuw nsw i64 %429, 2916
  %431 = add nuw nsw i64 %429, 5832
  %432 = add nuw nsw i64 %429, 8748
  %433 = add nuw nsw i64 %429, 11664
  %434 = add nuw nsw i64 %429, 14580
  %435 = add nuw nsw i64 %429, 17496
  %436 = add nuw nsw i64 %429, 20412
  %437 = shl i64 %indvars.iv197.1, 3
  %438 = add nuw nsw i64 %437, 216
  %439 = getelementptr inbounds [54 x <8 x float>], [54 x <8 x float>]* %4, i64 0, i64 0, i64 %438
  %440 = bitcast float* %439 to <8 x float>*
  %441 = load <8 x float>, <8 x float>* %440, align 32, !tbaa !49
  %442 = getelementptr inbounds float, float* %75, i64 %429
  %443 = extractelement <8 x float> %441, i64 0
  store float %443, float* %442, align 4, !tbaa !52
  %444 = getelementptr inbounds float, float* %75, i64 %430
  %445 = extractelement <8 x float> %441, i64 1
  store float %445, float* %444, align 4, !tbaa !52
  %446 = getelementptr inbounds float, float* %75, i64 %431
  %447 = extractelement <8 x float> %441, i64 2
  store float %447, float* %446, align 4, !tbaa !52
  %448 = getelementptr inbounds float, float* %75, i64 %432
  %449 = extractelement <8 x float> %441, i64 3
  store float %449, float* %448, align 4, !tbaa !52
  %450 = getelementptr inbounds float, float* %75, i64 %433
  %451 = extractelement <8 x float> %441, i64 4
  store float %451, float* %450, align 4, !tbaa !52
  %452 = getelementptr inbounds float, float* %75, i64 %434
  %453 = extractelement <8 x float> %441, i64 5
  store float %453, float* %452, align 4, !tbaa !52
  %454 = getelementptr inbounds float, float* %75, i64 %435
  %455 = extractelement <8 x float> %441, i64 6
  store float %455, float* %454, align 4, !tbaa !52
  %456 = getelementptr inbounds float, float* %75, i64 %436
  %457 = extractelement <8 x float> %441, i64 7
  store float %457, float* %456, align 4, !tbaa !52
  %indvars.iv.next198.1 = add nuw nsw i64 %indvars.iv197.1, 1
  %exitcond199.1 = icmp eq i64 %indvars.iv.next198.1, 27
  br i1 %exitcond199.1, label %for_end39.1, label %for_body38.1, !prof !28

for_end39.1:                                      ; preds = %for_body38.1
  %458 = add nuw nsw i32 %183, 1
  %exitcond203 = icmp eq i32 %458, 648
  br i1 %exitcond203, label %for_end18, label %for_begin19.preheader, !prof !28
}

; Function Attrs: nounwind readnone speculatable
declare <8 x float> @llvm.fmuladd.v8f32(<8 x float>, <8 x float>, <8 x float>) #4

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
  %2 = alloca [279936 x float], align 16
  %3 = alloca [291600 x float], align 16
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar37 = phi i64 [ 0, %entry ], [ %indvar.next38, %for_end3 ]
  %4 = phi i32 [ 0, %entry ], [ %13, %for_end3 ]
  %5 = mul nuw nsw i64 %indvar37, 2916
  %6 = mul nuw nsw i64 %indvar37, 11664
  %7 = add nsw i64 %6, -23328
  %.off = add nsw i32 %4, -2
  %8 = icmp ult i32 %.off, 96
  br i1 %8, label %for_begin4.preheader.us, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep43 = getelementptr [291600 x float], [291600 x float]* %3, i64 0, i64 %5
  %scevgep4344 = bitcast float* %scevgep43 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep4344, i8 0, i64 11664, i1 false)
  br label %for_end3

for_begin4.preheader.us:                          ; preds = %for_begin1.preheader, %for_begin4.preheader.us
  %indvar48 = phi i64 [ %indvar.next49, %for_begin4.preheader.us ], [ 0, %for_begin1.preheader ]
  %9 = mul nuw nsw i64 %indvar48, 54
  %10 = add nuw nsw i64 %5, %9
  %scevgep = getelementptr [291600 x float], [291600 x float]* %3, i64 0, i64 %10
  %scevgep50 = bitcast float* %scevgep to i8*
  %11 = mul nuw nsw i64 %indvar48, 216
  %12 = add nsw i64 %7, %11
  %scevgep51 = getelementptr i8, i8* %0, i64 %12
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep50, i8* align 4 %scevgep51, i64 216, i1 false)
  %indvar.next49 = add nuw nsw i64 %indvar48, 1
  %exitcond52 = icmp eq i64 %indvar.next49, 54
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
  %15 = mul nuw nsw i64 %indvar, 2916
  br label %for_begin13.preheader

for_begin13.preheader:                            ; preds = %for_begin13.preheader, %for_begin10.preheader
  %indvar29 = phi i64 [ 0, %for_begin10.preheader ], [ %indvar.next30, %for_begin13.preheader ]
  %16 = mul nuw nsw i64 %indvar29, 54
  %17 = add nuw nsw i64 %16, %15
  %18 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %17
  %19 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %17
  %20 = bitcast float* %19 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %20, align 8, !tbaa !55
  %21 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load, <4 x float> %wide.load, <4 x float> zeroinitializer)
  %22 = add nuw nsw i64 %17, 2916
  %23 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %22
  %24 = bitcast float* %23 to <4 x float>*
  %wide.load54 = load <4 x float>, <4 x float>* %24, align 8, !tbaa !55
  %25 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54, <4 x float> %wide.load54, <4 x float> %21)
  %26 = add nuw nsw i64 %17, 5832
  %27 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %26
  %28 = bitcast float* %27 to <4 x float>*
  %wide.load55 = load <4 x float>, <4 x float>* %28, align 8, !tbaa !55
  %29 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55, <4 x float> %wide.load55, <4 x float> %25)
  %30 = add nuw nsw i64 %17, 8748
  %31 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %30
  %32 = bitcast float* %31 to <4 x float>*
  %wide.load56 = load <4 x float>, <4 x float>* %32, align 8, !tbaa !55
  %33 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56, <4 x float> %wide.load56, <4 x float> %29)
  %34 = add nuw nsw i64 %17, 11664
  %35 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %34
  %36 = bitcast float* %35 to <4 x float>*
  %wide.load57 = load <4 x float>, <4 x float>* %36, align 8, !tbaa !55
  %37 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57, <4 x float> %wide.load57, <4 x float> %33)
  %38 = bitcast float* %18 to <4 x float>*
  store <4 x float> %37, <4 x float>* %38, align 8, !tbaa !58
  %39 = add nuw nsw i64 %17, 4
  %40 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %39
  %41 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %39
  %42 = bitcast float* %41 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %42, align 8, !tbaa !55
  %43 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.1, <4 x float> %wide.load.1, <4 x float> zeroinitializer)
  %44 = add nuw nsw i64 %17, 2920
  %45 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %44
  %46 = bitcast float* %45 to <4 x float>*
  %wide.load54.1 = load <4 x float>, <4 x float>* %46, align 8, !tbaa !55
  %47 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.1, <4 x float> %wide.load54.1, <4 x float> %43)
  %48 = add nuw nsw i64 %17, 5836
  %49 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %48
  %50 = bitcast float* %49 to <4 x float>*
  %wide.load55.1 = load <4 x float>, <4 x float>* %50, align 8, !tbaa !55
  %51 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55.1, <4 x float> %wide.load55.1, <4 x float> %47)
  %52 = add nuw nsw i64 %17, 8752
  %53 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %52
  %54 = bitcast float* %53 to <4 x float>*
  %wide.load56.1 = load <4 x float>, <4 x float>* %54, align 8, !tbaa !55
  %55 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56.1, <4 x float> %wide.load56.1, <4 x float> %51)
  %56 = add nuw nsw i64 %17, 11668
  %57 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %56
  %58 = bitcast float* %57 to <4 x float>*
  %wide.load57.1 = load <4 x float>, <4 x float>* %58, align 8, !tbaa !55
  %59 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57.1, <4 x float> %wide.load57.1, <4 x float> %55)
  %60 = bitcast float* %40 to <4 x float>*
  store <4 x float> %59, <4 x float>* %60, align 8, !tbaa !58
  %61 = add nuw nsw i64 %17, 8
  %62 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %61
  %63 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %61
  %64 = bitcast float* %63 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %64, align 8, !tbaa !55
  %65 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.2, <4 x float> %wide.load.2, <4 x float> zeroinitializer)
  %66 = add nuw nsw i64 %17, 2924
  %67 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %66
  %68 = bitcast float* %67 to <4 x float>*
  %wide.load54.2 = load <4 x float>, <4 x float>* %68, align 8, !tbaa !55
  %69 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.2, <4 x float> %wide.load54.2, <4 x float> %65)
  %70 = add nuw nsw i64 %17, 5840
  %71 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %70
  %72 = bitcast float* %71 to <4 x float>*
  %wide.load55.2 = load <4 x float>, <4 x float>* %72, align 8, !tbaa !55
  %73 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55.2, <4 x float> %wide.load55.2, <4 x float> %69)
  %74 = add nuw nsw i64 %17, 8756
  %75 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %74
  %76 = bitcast float* %75 to <4 x float>*
  %wide.load56.2 = load <4 x float>, <4 x float>* %76, align 8, !tbaa !55
  %77 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56.2, <4 x float> %wide.load56.2, <4 x float> %73)
  %78 = add nuw nsw i64 %17, 11672
  %79 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %78
  %80 = bitcast float* %79 to <4 x float>*
  %wide.load57.2 = load <4 x float>, <4 x float>* %80, align 8, !tbaa !55
  %81 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57.2, <4 x float> %wide.load57.2, <4 x float> %77)
  %82 = bitcast float* %62 to <4 x float>*
  store <4 x float> %81, <4 x float>* %82, align 8, !tbaa !58
  %83 = add nuw nsw i64 %17, 12
  %84 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %83
  %85 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %83
  %86 = bitcast float* %85 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %86, align 8, !tbaa !55
  %87 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.3, <4 x float> %wide.load.3, <4 x float> zeroinitializer)
  %88 = add nuw nsw i64 %17, 2928
  %89 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %88
  %90 = bitcast float* %89 to <4 x float>*
  %wide.load54.3 = load <4 x float>, <4 x float>* %90, align 8, !tbaa !55
  %91 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.3, <4 x float> %wide.load54.3, <4 x float> %87)
  %92 = add nuw nsw i64 %17, 5844
  %93 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %92
  %94 = bitcast float* %93 to <4 x float>*
  %wide.load55.3 = load <4 x float>, <4 x float>* %94, align 8, !tbaa !55
  %95 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55.3, <4 x float> %wide.load55.3, <4 x float> %91)
  %96 = add nuw nsw i64 %17, 8760
  %97 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %96
  %98 = bitcast float* %97 to <4 x float>*
  %wide.load56.3 = load <4 x float>, <4 x float>* %98, align 8, !tbaa !55
  %99 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56.3, <4 x float> %wide.load56.3, <4 x float> %95)
  %100 = add nuw nsw i64 %17, 11676
  %101 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %100
  %102 = bitcast float* %101 to <4 x float>*
  %wide.load57.3 = load <4 x float>, <4 x float>* %102, align 8, !tbaa !55
  %103 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57.3, <4 x float> %wide.load57.3, <4 x float> %99)
  %104 = bitcast float* %84 to <4 x float>*
  store <4 x float> %103, <4 x float>* %104, align 8, !tbaa !58
  %105 = add nuw nsw i64 %17, 16
  %106 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %105
  %107 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %105
  %108 = bitcast float* %107 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %108, align 8, !tbaa !55
  %109 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.4, <4 x float> %wide.load.4, <4 x float> zeroinitializer)
  %110 = add nuw nsw i64 %17, 2932
  %111 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %110
  %112 = bitcast float* %111 to <4 x float>*
  %wide.load54.4 = load <4 x float>, <4 x float>* %112, align 8, !tbaa !55
  %113 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.4, <4 x float> %wide.load54.4, <4 x float> %109)
  %114 = add nuw nsw i64 %17, 5848
  %115 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %114
  %116 = bitcast float* %115 to <4 x float>*
  %wide.load55.4 = load <4 x float>, <4 x float>* %116, align 8, !tbaa !55
  %117 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55.4, <4 x float> %wide.load55.4, <4 x float> %113)
  %118 = add nuw nsw i64 %17, 8764
  %119 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %118
  %120 = bitcast float* %119 to <4 x float>*
  %wide.load56.4 = load <4 x float>, <4 x float>* %120, align 8, !tbaa !55
  %121 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56.4, <4 x float> %wide.load56.4, <4 x float> %117)
  %122 = add nuw nsw i64 %17, 11680
  %123 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %122
  %124 = bitcast float* %123 to <4 x float>*
  %wide.load57.4 = load <4 x float>, <4 x float>* %124, align 8, !tbaa !55
  %125 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57.4, <4 x float> %wide.load57.4, <4 x float> %121)
  %126 = bitcast float* %106 to <4 x float>*
  store <4 x float> %125, <4 x float>* %126, align 8, !tbaa !58
  %127 = add nuw nsw i64 %17, 20
  %128 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %127
  %129 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %127
  %130 = bitcast float* %129 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %130, align 8, !tbaa !55
  %131 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.5, <4 x float> %wide.load.5, <4 x float> zeroinitializer)
  %132 = add nuw nsw i64 %17, 2936
  %133 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %132
  %134 = bitcast float* %133 to <4 x float>*
  %wide.load54.5 = load <4 x float>, <4 x float>* %134, align 8, !tbaa !55
  %135 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.5, <4 x float> %wide.load54.5, <4 x float> %131)
  %136 = add nuw nsw i64 %17, 5852
  %137 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %136
  %138 = bitcast float* %137 to <4 x float>*
  %wide.load55.5 = load <4 x float>, <4 x float>* %138, align 8, !tbaa !55
  %139 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55.5, <4 x float> %wide.load55.5, <4 x float> %135)
  %140 = add nuw nsw i64 %17, 8768
  %141 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %140
  %142 = bitcast float* %141 to <4 x float>*
  %wide.load56.5 = load <4 x float>, <4 x float>* %142, align 8, !tbaa !55
  %143 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56.5, <4 x float> %wide.load56.5, <4 x float> %139)
  %144 = add nuw nsw i64 %17, 11684
  %145 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %144
  %146 = bitcast float* %145 to <4 x float>*
  %wide.load57.5 = load <4 x float>, <4 x float>* %146, align 8, !tbaa !55
  %147 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57.5, <4 x float> %wide.load57.5, <4 x float> %143)
  %148 = bitcast float* %128 to <4 x float>*
  store <4 x float> %147, <4 x float>* %148, align 8, !tbaa !58
  %149 = add nuw nsw i64 %17, 24
  %150 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %149
  %151 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %149
  %152 = bitcast float* %151 to <4 x float>*
  %wide.load.6 = load <4 x float>, <4 x float>* %152, align 8, !tbaa !55
  %153 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.6, <4 x float> %wide.load.6, <4 x float> zeroinitializer)
  %154 = add nuw nsw i64 %17, 2940
  %155 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %154
  %156 = bitcast float* %155 to <4 x float>*
  %wide.load54.6 = load <4 x float>, <4 x float>* %156, align 8, !tbaa !55
  %157 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.6, <4 x float> %wide.load54.6, <4 x float> %153)
  %158 = add nuw nsw i64 %17, 5856
  %159 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %158
  %160 = bitcast float* %159 to <4 x float>*
  %wide.load55.6 = load <4 x float>, <4 x float>* %160, align 8, !tbaa !55
  %161 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55.6, <4 x float> %wide.load55.6, <4 x float> %157)
  %162 = add nuw nsw i64 %17, 8772
  %163 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %162
  %164 = bitcast float* %163 to <4 x float>*
  %wide.load56.6 = load <4 x float>, <4 x float>* %164, align 8, !tbaa !55
  %165 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56.6, <4 x float> %wide.load56.6, <4 x float> %161)
  %166 = add nuw nsw i64 %17, 11688
  %167 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %166
  %168 = bitcast float* %167 to <4 x float>*
  %wide.load57.6 = load <4 x float>, <4 x float>* %168, align 8, !tbaa !55
  %169 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57.6, <4 x float> %wide.load57.6, <4 x float> %165)
  %170 = bitcast float* %150 to <4 x float>*
  store <4 x float> %169, <4 x float>* %170, align 8, !tbaa !58
  %171 = add nuw nsw i64 %17, 28
  %172 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %171
  %173 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %171
  %174 = bitcast float* %173 to <4 x float>*
  %wide.load.7 = load <4 x float>, <4 x float>* %174, align 8, !tbaa !55
  %175 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.7, <4 x float> %wide.load.7, <4 x float> zeroinitializer)
  %176 = add nuw nsw i64 %17, 2944
  %177 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %176
  %178 = bitcast float* %177 to <4 x float>*
  %wide.load54.7 = load <4 x float>, <4 x float>* %178, align 8, !tbaa !55
  %179 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.7, <4 x float> %wide.load54.7, <4 x float> %175)
  %180 = add nuw nsw i64 %17, 5860
  %181 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %180
  %182 = bitcast float* %181 to <4 x float>*
  %wide.load55.7 = load <4 x float>, <4 x float>* %182, align 8, !tbaa !55
  %183 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55.7, <4 x float> %wide.load55.7, <4 x float> %179)
  %184 = add nuw nsw i64 %17, 8776
  %185 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %184
  %186 = bitcast float* %185 to <4 x float>*
  %wide.load56.7 = load <4 x float>, <4 x float>* %186, align 8, !tbaa !55
  %187 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56.7, <4 x float> %wide.load56.7, <4 x float> %183)
  %188 = add nuw nsw i64 %17, 11692
  %189 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %188
  %190 = bitcast float* %189 to <4 x float>*
  %wide.load57.7 = load <4 x float>, <4 x float>* %190, align 8, !tbaa !55
  %191 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57.7, <4 x float> %wide.load57.7, <4 x float> %187)
  %192 = bitcast float* %172 to <4 x float>*
  store <4 x float> %191, <4 x float>* %192, align 8, !tbaa !58
  %193 = add nuw nsw i64 %17, 32
  %194 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %193
  %195 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %193
  %196 = bitcast float* %195 to <4 x float>*
  %wide.load.8 = load <4 x float>, <4 x float>* %196, align 8, !tbaa !55
  %197 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.8, <4 x float> %wide.load.8, <4 x float> zeroinitializer)
  %198 = add nuw nsw i64 %17, 2948
  %199 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %198
  %200 = bitcast float* %199 to <4 x float>*
  %wide.load54.8 = load <4 x float>, <4 x float>* %200, align 8, !tbaa !55
  %201 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.8, <4 x float> %wide.load54.8, <4 x float> %197)
  %202 = add nuw nsw i64 %17, 5864
  %203 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %202
  %204 = bitcast float* %203 to <4 x float>*
  %wide.load55.8 = load <4 x float>, <4 x float>* %204, align 8, !tbaa !55
  %205 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55.8, <4 x float> %wide.load55.8, <4 x float> %201)
  %206 = add nuw nsw i64 %17, 8780
  %207 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %206
  %208 = bitcast float* %207 to <4 x float>*
  %wide.load56.8 = load <4 x float>, <4 x float>* %208, align 8, !tbaa !55
  %209 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56.8, <4 x float> %wide.load56.8, <4 x float> %205)
  %210 = add nuw nsw i64 %17, 11696
  %211 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %210
  %212 = bitcast float* %211 to <4 x float>*
  %wide.load57.8 = load <4 x float>, <4 x float>* %212, align 8, !tbaa !55
  %213 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57.8, <4 x float> %wide.load57.8, <4 x float> %209)
  %214 = bitcast float* %194 to <4 x float>*
  store <4 x float> %213, <4 x float>* %214, align 8, !tbaa !58
  %215 = add nuw nsw i64 %17, 36
  %216 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %215
  %217 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %215
  %218 = bitcast float* %217 to <4 x float>*
  %wide.load.9 = load <4 x float>, <4 x float>* %218, align 8, !tbaa !55
  %219 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.9, <4 x float> %wide.load.9, <4 x float> zeroinitializer)
  %220 = add nuw nsw i64 %17, 2952
  %221 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %220
  %222 = bitcast float* %221 to <4 x float>*
  %wide.load54.9 = load <4 x float>, <4 x float>* %222, align 8, !tbaa !55
  %223 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.9, <4 x float> %wide.load54.9, <4 x float> %219)
  %224 = add nuw nsw i64 %17, 5868
  %225 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %224
  %226 = bitcast float* %225 to <4 x float>*
  %wide.load55.9 = load <4 x float>, <4 x float>* %226, align 8, !tbaa !55
  %227 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55.9, <4 x float> %wide.load55.9, <4 x float> %223)
  %228 = add nuw nsw i64 %17, 8784
  %229 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %228
  %230 = bitcast float* %229 to <4 x float>*
  %wide.load56.9 = load <4 x float>, <4 x float>* %230, align 8, !tbaa !55
  %231 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56.9, <4 x float> %wide.load56.9, <4 x float> %227)
  %232 = add nuw nsw i64 %17, 11700
  %233 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %232
  %234 = bitcast float* %233 to <4 x float>*
  %wide.load57.9 = load <4 x float>, <4 x float>* %234, align 8, !tbaa !55
  %235 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57.9, <4 x float> %wide.load57.9, <4 x float> %231)
  %236 = bitcast float* %216 to <4 x float>*
  store <4 x float> %235, <4 x float>* %236, align 8, !tbaa !58
  %237 = add nuw nsw i64 %17, 40
  %238 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %237
  %239 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %237
  %240 = bitcast float* %239 to <4 x float>*
  %wide.load.10 = load <4 x float>, <4 x float>* %240, align 8, !tbaa !55
  %241 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.10, <4 x float> %wide.load.10, <4 x float> zeroinitializer)
  %242 = add nuw nsw i64 %17, 2956
  %243 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %242
  %244 = bitcast float* %243 to <4 x float>*
  %wide.load54.10 = load <4 x float>, <4 x float>* %244, align 8, !tbaa !55
  %245 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.10, <4 x float> %wide.load54.10, <4 x float> %241)
  %246 = add nuw nsw i64 %17, 5872
  %247 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %246
  %248 = bitcast float* %247 to <4 x float>*
  %wide.load55.10 = load <4 x float>, <4 x float>* %248, align 8, !tbaa !55
  %249 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55.10, <4 x float> %wide.load55.10, <4 x float> %245)
  %250 = add nuw nsw i64 %17, 8788
  %251 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %250
  %252 = bitcast float* %251 to <4 x float>*
  %wide.load56.10 = load <4 x float>, <4 x float>* %252, align 8, !tbaa !55
  %253 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56.10, <4 x float> %wide.load56.10, <4 x float> %249)
  %254 = add nuw nsw i64 %17, 11704
  %255 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %254
  %256 = bitcast float* %255 to <4 x float>*
  %wide.load57.10 = load <4 x float>, <4 x float>* %256, align 8, !tbaa !55
  %257 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57.10, <4 x float> %wide.load57.10, <4 x float> %253)
  %258 = bitcast float* %238 to <4 x float>*
  store <4 x float> %257, <4 x float>* %258, align 8, !tbaa !58
  %259 = add nuw nsw i64 %17, 44
  %260 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %259
  %261 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %259
  %262 = bitcast float* %261 to <4 x float>*
  %wide.load.11 = load <4 x float>, <4 x float>* %262, align 8, !tbaa !55
  %263 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.11, <4 x float> %wide.load.11, <4 x float> zeroinitializer)
  %264 = add nuw nsw i64 %17, 2960
  %265 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %264
  %266 = bitcast float* %265 to <4 x float>*
  %wide.load54.11 = load <4 x float>, <4 x float>* %266, align 8, !tbaa !55
  %267 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.11, <4 x float> %wide.load54.11, <4 x float> %263)
  %268 = add nuw nsw i64 %17, 5876
  %269 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %268
  %270 = bitcast float* %269 to <4 x float>*
  %wide.load55.11 = load <4 x float>, <4 x float>* %270, align 8, !tbaa !55
  %271 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55.11, <4 x float> %wide.load55.11, <4 x float> %267)
  %272 = add nuw nsw i64 %17, 8792
  %273 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %272
  %274 = bitcast float* %273 to <4 x float>*
  %wide.load56.11 = load <4 x float>, <4 x float>* %274, align 8, !tbaa !55
  %275 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56.11, <4 x float> %wide.load56.11, <4 x float> %271)
  %276 = add nuw nsw i64 %17, 11708
  %277 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %276
  %278 = bitcast float* %277 to <4 x float>*
  %wide.load57.11 = load <4 x float>, <4 x float>* %278, align 8, !tbaa !55
  %279 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57.11, <4 x float> %wide.load57.11, <4 x float> %275)
  %280 = bitcast float* %260 to <4 x float>*
  store <4 x float> %279, <4 x float>* %280, align 8, !tbaa !58
  %281 = add nuw nsw i64 %17, 48
  %282 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %281
  %283 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %281
  %284 = bitcast float* %283 to <4 x float>*
  %wide.load.12 = load <4 x float>, <4 x float>* %284, align 8, !tbaa !55
  %285 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.12, <4 x float> %wide.load.12, <4 x float> zeroinitializer)
  %286 = add nuw nsw i64 %17, 2964
  %287 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %286
  %288 = bitcast float* %287 to <4 x float>*
  %wide.load54.12 = load <4 x float>, <4 x float>* %288, align 8, !tbaa !55
  %289 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.12, <4 x float> %wide.load54.12, <4 x float> %285)
  %290 = add nuw nsw i64 %17, 5880
  %291 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %290
  %292 = bitcast float* %291 to <4 x float>*
  %wide.load55.12 = load <4 x float>, <4 x float>* %292, align 8, !tbaa !55
  %293 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load55.12, <4 x float> %wide.load55.12, <4 x float> %289)
  %294 = add nuw nsw i64 %17, 8796
  %295 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %294
  %296 = bitcast float* %295 to <4 x float>*
  %wide.load56.12 = load <4 x float>, <4 x float>* %296, align 8, !tbaa !55
  %297 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load56.12, <4 x float> %wide.load56.12, <4 x float> %293)
  %298 = add nuw nsw i64 %17, 11712
  %299 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %298
  %300 = bitcast float* %299 to <4 x float>*
  %wide.load57.12 = load <4 x float>, <4 x float>* %300, align 8, !tbaa !55
  %301 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load57.12, <4 x float> %wide.load57.12, <4 x float> %297)
  %302 = bitcast float* %282 to <4 x float>*
  store <4 x float> %301, <4 x float>* %302, align 8, !tbaa !58
  %303 = add nuw nsw i64 %17, 52
  %304 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %303
  %305 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %303
  %306 = load float, float* %305, align 8, !tbaa !55
  %307 = tail call float @llvm.fmuladd.f32(float %306, float %306, float 0.000000e+00)
  %308 = add nuw nsw i64 %17, 2968
  %309 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %308
  %310 = load float, float* %309, align 8, !tbaa !55
  %311 = tail call float @llvm.fmuladd.f32(float %310, float %310, float %307)
  %312 = add nuw nsw i64 %17, 5884
  %313 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %312
  %314 = load float, float* %313, align 8, !tbaa !55
  %315 = tail call float @llvm.fmuladd.f32(float %314, float %314, float %311)
  %316 = add nuw nsw i64 %17, 8800
  %317 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %316
  %318 = load float, float* %317, align 8, !tbaa !55
  %319 = tail call float @llvm.fmuladd.f32(float %318, float %318, float %315)
  %320 = add nuw nsw i64 %17, 11716
  %321 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %320
  %322 = load float, float* %321, align 8, !tbaa !55
  %323 = tail call float @llvm.fmuladd.f32(float %322, float %322, float %319)
  store float %323, float* %304, align 8, !tbaa !58
  %324 = add nuw nsw i64 %17, 53
  %325 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %324
  %326 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %324
  %327 = load float, float* %326, align 4, !tbaa !55
  %328 = tail call float @llvm.fmuladd.f32(float %327, float %327, float 0.000000e+00)
  %329 = add nuw nsw i64 %17, 2969
  %330 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %329
  %331 = load float, float* %330, align 4, !tbaa !55
  %332 = tail call float @llvm.fmuladd.f32(float %331, float %331, float %328)
  %333 = add nuw nsw i64 %17, 5885
  %334 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %333
  %335 = load float, float* %334, align 4, !tbaa !55
  %336 = tail call float @llvm.fmuladd.f32(float %335, float %335, float %332)
  %337 = add nuw nsw i64 %17, 8801
  %338 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %337
  %339 = load float, float* %338, align 4, !tbaa !55
  %340 = tail call float @llvm.fmuladd.f32(float %339, float %339, float %336)
  %341 = add nuw nsw i64 %17, 11717
  %342 = getelementptr inbounds [291600 x float], [291600 x float]* %3, i64 0, i64 %341
  %343 = load float, float* %342, align 4, !tbaa !55
  %344 = tail call float @llvm.fmuladd.f32(float %343, float %343, float %340)
  store float %344, float* %325, align 4, !tbaa !58
  %indvar.next30 = add nuw nsw i64 %indvar29, 1
  %exitcond32 = icmp eq i64 %indvar.next30, 54
  br i1 %exitcond32, label %for_end12, label %for_begin13.preheader, !prof !28

for_end12:                                        ; preds = %for_begin13.preheader
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond33 = icmp eq i64 %indvar.next, 96
  br i1 %exitcond33, label %for_begin22.preheader, label %for_begin10.preheader, !prof !28

for_begin22.preheader:                            ; preds = %for_end12, %for_end24
  %indvars.iv20 = phi i64 [ %indvars.iv.next21, %for_end24 ], [ 0, %for_end12 ]
  %345 = mul nuw nsw i64 %indvars.iv20, 2916
  br label %for_begin25.preheader

for_begin28.preheader:                            ; preds = %for_end24
  %346 = bitcast i8* %1 to float*
  br label %for_begin31.preheader

for_begin25.preheader:                            ; preds = %for_begin25.preheader, %for_begin22.preheader
  %indvars.iv17 = phi i64 [ 0, %for_begin22.preheader ], [ %indvars.iv.next18, %for_begin25.preheader ]
  %347 = mul nuw nsw i64 %indvars.iv17, 54
  %348 = add nuw nsw i64 %347, %345
  %349 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %348
  %350 = bitcast float* %349 to <4 x float>*
  %wide.load69 = load <4 x float>, <4 x float>* %350, align 8, !tbaa !58
  %351 = fmul <4 x float> %wide.load69, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %352 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %351, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %353 = call <4 x float> @llvm.pow.v4f32(<4 x float> %352, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %354 = bitcast float* %349 to <4 x float>*
  store <4 x float> %353, <4 x float>* %354, align 8, !tbaa !58
  %355 = add nuw nsw i64 %348, 4
  %356 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %355
  %357 = bitcast float* %356 to <4 x float>*
  %wide.load69.1 = load <4 x float>, <4 x float>* %357, align 8, !tbaa !58
  %358 = fmul <4 x float> %wide.load69.1, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %359 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %358, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %360 = call <4 x float> @llvm.pow.v4f32(<4 x float> %359, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %361 = bitcast float* %356 to <4 x float>*
  store <4 x float> %360, <4 x float>* %361, align 8, !tbaa !58
  %362 = add nuw nsw i64 %348, 8
  %363 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %362
  %364 = bitcast float* %363 to <4 x float>*
  %wide.load69.2 = load <4 x float>, <4 x float>* %364, align 8, !tbaa !58
  %365 = fmul <4 x float> %wide.load69.2, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %366 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %365, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %367 = call <4 x float> @llvm.pow.v4f32(<4 x float> %366, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %368 = bitcast float* %363 to <4 x float>*
  store <4 x float> %367, <4 x float>* %368, align 8, !tbaa !58
  %369 = add nuw nsw i64 %348, 12
  %370 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %369
  %371 = bitcast float* %370 to <4 x float>*
  %wide.load69.3 = load <4 x float>, <4 x float>* %371, align 8, !tbaa !58
  %372 = fmul <4 x float> %wide.load69.3, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %373 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %372, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %374 = call <4 x float> @llvm.pow.v4f32(<4 x float> %373, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %375 = bitcast float* %370 to <4 x float>*
  store <4 x float> %374, <4 x float>* %375, align 8, !tbaa !58
  %376 = add nuw nsw i64 %348, 16
  %377 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %376
  %378 = bitcast float* %377 to <4 x float>*
  %wide.load69.4 = load <4 x float>, <4 x float>* %378, align 8, !tbaa !58
  %379 = fmul <4 x float> %wide.load69.4, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %380 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %379, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %381 = call <4 x float> @llvm.pow.v4f32(<4 x float> %380, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %382 = bitcast float* %377 to <4 x float>*
  store <4 x float> %381, <4 x float>* %382, align 8, !tbaa !58
  %383 = add nuw nsw i64 %348, 20
  %384 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %383
  %385 = bitcast float* %384 to <4 x float>*
  %wide.load69.5 = load <4 x float>, <4 x float>* %385, align 8, !tbaa !58
  %386 = fmul <4 x float> %wide.load69.5, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %387 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %386, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %388 = call <4 x float> @llvm.pow.v4f32(<4 x float> %387, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %389 = bitcast float* %384 to <4 x float>*
  store <4 x float> %388, <4 x float>* %389, align 8, !tbaa !58
  %390 = add nuw nsw i64 %348, 24
  %391 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %390
  %392 = bitcast float* %391 to <4 x float>*
  %wide.load69.6 = load <4 x float>, <4 x float>* %392, align 8, !tbaa !58
  %393 = fmul <4 x float> %wide.load69.6, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %394 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %393, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %395 = call <4 x float> @llvm.pow.v4f32(<4 x float> %394, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %396 = bitcast float* %391 to <4 x float>*
  store <4 x float> %395, <4 x float>* %396, align 8, !tbaa !58
  %397 = add nuw nsw i64 %348, 28
  %398 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %397
  %399 = bitcast float* %398 to <4 x float>*
  %wide.load69.7 = load <4 x float>, <4 x float>* %399, align 8, !tbaa !58
  %400 = fmul <4 x float> %wide.load69.7, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %401 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %400, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %402 = call <4 x float> @llvm.pow.v4f32(<4 x float> %401, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %403 = bitcast float* %398 to <4 x float>*
  store <4 x float> %402, <4 x float>* %403, align 8, !tbaa !58
  %404 = add nuw nsw i64 %348, 32
  %405 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %404
  %406 = bitcast float* %405 to <4 x float>*
  %wide.load69.8 = load <4 x float>, <4 x float>* %406, align 8, !tbaa !58
  %407 = fmul <4 x float> %wide.load69.8, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %408 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %407, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %409 = call <4 x float> @llvm.pow.v4f32(<4 x float> %408, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %410 = bitcast float* %405 to <4 x float>*
  store <4 x float> %409, <4 x float>* %410, align 8, !tbaa !58
  %411 = add nuw nsw i64 %348, 36
  %412 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %411
  %413 = bitcast float* %412 to <4 x float>*
  %wide.load69.9 = load <4 x float>, <4 x float>* %413, align 8, !tbaa !58
  %414 = fmul <4 x float> %wide.load69.9, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %415 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %414, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %416 = call <4 x float> @llvm.pow.v4f32(<4 x float> %415, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %417 = bitcast float* %412 to <4 x float>*
  store <4 x float> %416, <4 x float>* %417, align 8, !tbaa !58
  %418 = add nuw nsw i64 %348, 40
  %419 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %418
  %420 = bitcast float* %419 to <4 x float>*
  %wide.load69.10 = load <4 x float>, <4 x float>* %420, align 8, !tbaa !58
  %421 = fmul <4 x float> %wide.load69.10, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %422 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %421, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %423 = call <4 x float> @llvm.pow.v4f32(<4 x float> %422, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %424 = bitcast float* %419 to <4 x float>*
  store <4 x float> %423, <4 x float>* %424, align 8, !tbaa !58
  %425 = add nuw nsw i64 %348, 44
  %426 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %425
  %427 = bitcast float* %426 to <4 x float>*
  %wide.load69.11 = load <4 x float>, <4 x float>* %427, align 8, !tbaa !58
  %428 = fmul <4 x float> %wide.load69.11, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %429 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %428, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %430 = call <4 x float> @llvm.pow.v4f32(<4 x float> %429, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %431 = bitcast float* %426 to <4 x float>*
  store <4 x float> %430, <4 x float>* %431, align 8, !tbaa !58
  %432 = add nuw nsw i64 %348, 48
  %433 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %432
  %434 = bitcast float* %433 to <4 x float>*
  %wide.load69.12 = load <4 x float>, <4 x float>* %434, align 8, !tbaa !58
  %435 = fmul <4 x float> %wide.load69.12, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %436 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %435, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %437 = call <4 x float> @llvm.pow.v4f32(<4 x float> %436, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %438 = bitcast float* %433 to <4 x float>*
  store <4 x float> %437, <4 x float>* %438, align 8, !tbaa !58
  %439 = add nuw nsw i64 %348, 52
  %440 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %439
  %441 = load float, float* %440, align 8, !tbaa !58
  %442 = fmul float %441, 0x3F1A36E2E0000000
  %443 = tail call float @llvm.fmuladd.f32(float %442, float 0x3FC99999A0000000, float 1.000000e+00)
  %444 = tail call float @llvm.pow.f32(float %443, float 7.500000e-01)
  store float %444, float* %440, align 8, !tbaa !58
  %445 = add nuw nsw i64 %348, 53
  %446 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %445
  %447 = load float, float* %446, align 4, !tbaa !58
  %448 = fmul float %447, 0x3F1A36E2E0000000
  %449 = tail call float @llvm.fmuladd.f32(float %448, float 0x3FC99999A0000000, float 1.000000e+00)
  %450 = tail call float @llvm.pow.f32(float %449, float 7.500000e-01)
  store float %450, float* %446, align 4, !tbaa !58
  %indvars.iv.next18 = add nuw nsw i64 %indvars.iv17, 1
  %exitcond19 = icmp eq i64 %indvars.iv.next18, 54
  br i1 %exitcond19, label %for_end24, label %for_begin25.preheader, !prof !28

for_end24:                                        ; preds = %for_begin25.preheader
  %indvars.iv.next21 = add nuw nsw i64 %indvars.iv20, 1
  %exitcond22 = icmp eq i64 %indvars.iv.next21, 96
  br i1 %exitcond22, label %for_begin28.preheader, label %for_begin22.preheader, !prof !28

for_begin31.preheader:                            ; preds = %for_end33, %for_begin28.preheader
  %indvars.iv11 = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next12, %for_end33 ]
  %451 = mul nuw nsw i64 %indvars.iv11, 2916
  br label %for_begin34.preheader

for_end30:                                        ; preds = %for_end33
  ret void

for_begin34.preheader:                            ; preds = %for_begin34.preheader, %for_begin31.preheader
  %indvars.iv8 = phi i64 [ 0, %for_begin31.preheader ], [ %indvars.iv.next9, %for_begin34.preheader ]
  %452 = mul nuw nsw i64 %indvars.iv8, 54
  %453 = add nuw nsw i64 %452, %451
  %454 = getelementptr inbounds float, float* %14, i64 %453
  %455 = bitcast float* %454 to <4 x float>*
  %wide.load81 = load <4 x float>, <4 x float>* %455, align 4, !tbaa !61
  %456 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %453
  %457 = bitcast float* %456 to <4 x float>*
  %wide.load82 = load <4 x float>, <4 x float>* %457, align 8, !tbaa !58
  %458 = fdiv <4 x float> %wide.load81, %wide.load82
  %459 = getelementptr inbounds float, float* %346, i64 %453
  %460 = bitcast float* %459 to <4 x float>*
  store <4 x float> %458, <4 x float>* %460, align 4, !tbaa !64
  %461 = add nuw nsw i64 %453, 4
  %462 = getelementptr inbounds float, float* %14, i64 %461
  %463 = bitcast float* %462 to <4 x float>*
  %wide.load81.1 = load <4 x float>, <4 x float>* %463, align 4, !tbaa !61
  %464 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %461
  %465 = bitcast float* %464 to <4 x float>*
  %wide.load82.1 = load <4 x float>, <4 x float>* %465, align 8, !tbaa !58
  %466 = fdiv <4 x float> %wide.load81.1, %wide.load82.1
  %467 = getelementptr inbounds float, float* %346, i64 %461
  %468 = bitcast float* %467 to <4 x float>*
  store <4 x float> %466, <4 x float>* %468, align 4, !tbaa !64
  %469 = add nuw nsw i64 %453, 8
  %470 = getelementptr inbounds float, float* %14, i64 %469
  %471 = bitcast float* %470 to <4 x float>*
  %wide.load81.2 = load <4 x float>, <4 x float>* %471, align 4, !tbaa !61
  %472 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %469
  %473 = bitcast float* %472 to <4 x float>*
  %wide.load82.2 = load <4 x float>, <4 x float>* %473, align 8, !tbaa !58
  %474 = fdiv <4 x float> %wide.load81.2, %wide.load82.2
  %475 = getelementptr inbounds float, float* %346, i64 %469
  %476 = bitcast float* %475 to <4 x float>*
  store <4 x float> %474, <4 x float>* %476, align 4, !tbaa !64
  %477 = add nuw nsw i64 %453, 12
  %478 = getelementptr inbounds float, float* %14, i64 %477
  %479 = bitcast float* %478 to <4 x float>*
  %wide.load81.3 = load <4 x float>, <4 x float>* %479, align 4, !tbaa !61
  %480 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %477
  %481 = bitcast float* %480 to <4 x float>*
  %wide.load82.3 = load <4 x float>, <4 x float>* %481, align 8, !tbaa !58
  %482 = fdiv <4 x float> %wide.load81.3, %wide.load82.3
  %483 = getelementptr inbounds float, float* %346, i64 %477
  %484 = bitcast float* %483 to <4 x float>*
  store <4 x float> %482, <4 x float>* %484, align 4, !tbaa !64
  %485 = add nuw nsw i64 %453, 16
  %486 = getelementptr inbounds float, float* %14, i64 %485
  %487 = bitcast float* %486 to <4 x float>*
  %wide.load81.4 = load <4 x float>, <4 x float>* %487, align 4, !tbaa !61
  %488 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %485
  %489 = bitcast float* %488 to <4 x float>*
  %wide.load82.4 = load <4 x float>, <4 x float>* %489, align 8, !tbaa !58
  %490 = fdiv <4 x float> %wide.load81.4, %wide.load82.4
  %491 = getelementptr inbounds float, float* %346, i64 %485
  %492 = bitcast float* %491 to <4 x float>*
  store <4 x float> %490, <4 x float>* %492, align 4, !tbaa !64
  %493 = add nuw nsw i64 %453, 20
  %494 = getelementptr inbounds float, float* %14, i64 %493
  %495 = bitcast float* %494 to <4 x float>*
  %wide.load81.5 = load <4 x float>, <4 x float>* %495, align 4, !tbaa !61
  %496 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %493
  %497 = bitcast float* %496 to <4 x float>*
  %wide.load82.5 = load <4 x float>, <4 x float>* %497, align 8, !tbaa !58
  %498 = fdiv <4 x float> %wide.load81.5, %wide.load82.5
  %499 = getelementptr inbounds float, float* %346, i64 %493
  %500 = bitcast float* %499 to <4 x float>*
  store <4 x float> %498, <4 x float>* %500, align 4, !tbaa !64
  %501 = add nuw nsw i64 %453, 24
  %502 = getelementptr inbounds float, float* %14, i64 %501
  %503 = bitcast float* %502 to <4 x float>*
  %wide.load81.6 = load <4 x float>, <4 x float>* %503, align 4, !tbaa !61
  %504 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %501
  %505 = bitcast float* %504 to <4 x float>*
  %wide.load82.6 = load <4 x float>, <4 x float>* %505, align 8, !tbaa !58
  %506 = fdiv <4 x float> %wide.load81.6, %wide.load82.6
  %507 = getelementptr inbounds float, float* %346, i64 %501
  %508 = bitcast float* %507 to <4 x float>*
  store <4 x float> %506, <4 x float>* %508, align 4, !tbaa !64
  %509 = add nuw nsw i64 %453, 28
  %510 = getelementptr inbounds float, float* %14, i64 %509
  %511 = bitcast float* %510 to <4 x float>*
  %wide.load81.7 = load <4 x float>, <4 x float>* %511, align 4, !tbaa !61
  %512 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %509
  %513 = bitcast float* %512 to <4 x float>*
  %wide.load82.7 = load <4 x float>, <4 x float>* %513, align 8, !tbaa !58
  %514 = fdiv <4 x float> %wide.load81.7, %wide.load82.7
  %515 = getelementptr inbounds float, float* %346, i64 %509
  %516 = bitcast float* %515 to <4 x float>*
  store <4 x float> %514, <4 x float>* %516, align 4, !tbaa !64
  %517 = add nuw nsw i64 %453, 32
  %518 = getelementptr inbounds float, float* %14, i64 %517
  %519 = bitcast float* %518 to <4 x float>*
  %wide.load81.8 = load <4 x float>, <4 x float>* %519, align 4, !tbaa !61
  %520 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %517
  %521 = bitcast float* %520 to <4 x float>*
  %wide.load82.8 = load <4 x float>, <4 x float>* %521, align 8, !tbaa !58
  %522 = fdiv <4 x float> %wide.load81.8, %wide.load82.8
  %523 = getelementptr inbounds float, float* %346, i64 %517
  %524 = bitcast float* %523 to <4 x float>*
  store <4 x float> %522, <4 x float>* %524, align 4, !tbaa !64
  %525 = add nuw nsw i64 %453, 36
  %526 = getelementptr inbounds float, float* %14, i64 %525
  %527 = bitcast float* %526 to <4 x float>*
  %wide.load81.9 = load <4 x float>, <4 x float>* %527, align 4, !tbaa !61
  %528 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %525
  %529 = bitcast float* %528 to <4 x float>*
  %wide.load82.9 = load <4 x float>, <4 x float>* %529, align 8, !tbaa !58
  %530 = fdiv <4 x float> %wide.load81.9, %wide.load82.9
  %531 = getelementptr inbounds float, float* %346, i64 %525
  %532 = bitcast float* %531 to <4 x float>*
  store <4 x float> %530, <4 x float>* %532, align 4, !tbaa !64
  %533 = add nuw nsw i64 %453, 40
  %534 = getelementptr inbounds float, float* %14, i64 %533
  %535 = bitcast float* %534 to <4 x float>*
  %wide.load81.10 = load <4 x float>, <4 x float>* %535, align 4, !tbaa !61
  %536 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %533
  %537 = bitcast float* %536 to <4 x float>*
  %wide.load82.10 = load <4 x float>, <4 x float>* %537, align 8, !tbaa !58
  %538 = fdiv <4 x float> %wide.load81.10, %wide.load82.10
  %539 = getelementptr inbounds float, float* %346, i64 %533
  %540 = bitcast float* %539 to <4 x float>*
  store <4 x float> %538, <4 x float>* %540, align 4, !tbaa !64
  %541 = add nuw nsw i64 %453, 44
  %542 = getelementptr inbounds float, float* %14, i64 %541
  %543 = bitcast float* %542 to <4 x float>*
  %wide.load81.11 = load <4 x float>, <4 x float>* %543, align 4, !tbaa !61
  %544 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %541
  %545 = bitcast float* %544 to <4 x float>*
  %wide.load82.11 = load <4 x float>, <4 x float>* %545, align 8, !tbaa !58
  %546 = fdiv <4 x float> %wide.load81.11, %wide.load82.11
  %547 = getelementptr inbounds float, float* %346, i64 %541
  %548 = bitcast float* %547 to <4 x float>*
  store <4 x float> %546, <4 x float>* %548, align 4, !tbaa !64
  %549 = add nuw nsw i64 %453, 48
  %550 = getelementptr inbounds float, float* %14, i64 %549
  %551 = bitcast float* %550 to <4 x float>*
  %wide.load81.12 = load <4 x float>, <4 x float>* %551, align 4, !tbaa !61
  %552 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %549
  %553 = bitcast float* %552 to <4 x float>*
  %wide.load82.12 = load <4 x float>, <4 x float>* %553, align 8, !tbaa !58
  %554 = fdiv <4 x float> %wide.load81.12, %wide.load82.12
  %555 = getelementptr inbounds float, float* %346, i64 %549
  %556 = bitcast float* %555 to <4 x float>*
  store <4 x float> %554, <4 x float>* %556, align 4, !tbaa !64
  %557 = add nuw nsw i64 %453, 52
  %558 = getelementptr inbounds float, float* %14, i64 %557
  %559 = load float, float* %558, align 4, !tbaa !61
  %560 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %557
  %561 = load float, float* %560, align 8, !tbaa !58
  %562 = fdiv float %559, %561
  %563 = getelementptr inbounds float, float* %346, i64 %557
  store float %562, float* %563, align 4, !tbaa !64
  %564 = add nuw nsw i64 %453, 53
  %565 = getelementptr inbounds float, float* %14, i64 %564
  %566 = load float, float* %565, align 4, !tbaa !61
  %567 = getelementptr inbounds [279936 x float], [279936 x float]* %2, i64 0, i64 %564
  %568 = load float, float* %567, align 4, !tbaa !58
  %569 = fdiv float %566, %568
  %570 = getelementptr inbounds float, float* %346, i64 %564
  store float %569, float* %570, align 4, !tbaa !64
  %indvars.iv.next9 = add nuw nsw i64 %indvars.iv8, 1
  %exitcond10 = icmp eq i64 %indvars.iv.next9, 54
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
  tail call fastcc void @fused_nn_conv2d_3_compute_(i8* %12, i8* %16, i8* %14)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_conv2d_3_compute_(i8* noalias nocapture readonly, i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #3 {
entry:
  %3 = alloca [86400 x float], align 16
  %4 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar18 = phi i64 [ 0, %entry ], [ %indvar.next19, %for_end3 ]
  %5 = mul nuw nsw i64 %indvar18, 900
  %6 = mul nuw nsw i64 %indvar18, 676
  %7 = add nsw i64 %6, -54
  br label %for_begin4.preheader

for_begin7.preheader:                             ; preds = %for_end3
  %8 = bitcast i8* %1 to float*
  %9 = bitcast i8* %2 to float*
  br label %for_begin10.preheader

for_begin4.preheader:                             ; preds = %for_end6, %for_begin1.preheader
  %indvar20 = phi i64 [ 0, %for_begin1.preheader ], [ %indvar.next21, %for_end6 ]
  %10 = mul nuw nsw i64 %indvar20, 30
  %11 = add nuw nsw i64 %5, %10
  %12 = trunc i64 %indvar20 to i32
  %13 = add i32 %12, -2
  %14 = icmp ult i32 %13, 26
  %15 = mul nuw nsw i64 %indvar20, 26
  %16 = add nsw i64 %7, %15
  br i1 %14, label %for_body5.us, label %for_body5.preheader

for_body5.preheader:                              ; preds = %for_begin4.preheader
  %scevgep = getelementptr [86400 x float], [86400 x float]* %3, i64 0, i64 %11
  %scevgep22 = bitcast float* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %scevgep22, i8 0, i64 120, i1 false)
  br label %for_end6

for_body5.us:                                     ; preds = %for_begin4.preheader, %if_end.us
  %indvars.iv23 = phi i64 [ %indvars.iv.next24, %if_end.us ], [ 0, %for_begin4.preheader ]
  %17 = add nuw nsw i64 %11, %indvars.iv23
  %18 = trunc i64 %indvars.iv23 to i32
  %19 = add i32 %18, -2
  %20 = icmp ult i32 %19, 26
  br i1 %20, label %if_then.us, label %if_end.us

if_then.us:                                       ; preds = %for_body5.us
  %21 = add nsw i64 %16, %indvars.iv23
  %22 = getelementptr inbounds float, float* %4, i64 %21
  %23 = load float, float* %22, align 4, !tbaa !67
  br label %if_end.us

if_end.us:                                        ; preds = %if_then.us, %for_body5.us
  %24 = phi float [ %23, %if_then.us ], [ 0.000000e+00, %for_body5.us ]
  %25 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %17
  store float %24, float* %25, align 4, !tbaa !70
  %indvars.iv.next24 = add nuw nsw i64 %indvars.iv23, 1
  %exitcond26 = icmp eq i64 %indvars.iv.next24, 30
  br i1 %exitcond26, label %for_end6, label %for_body5.us, !prof !28

for_end3:                                         ; preds = %for_end6
  %indvar.next19 = add nuw nsw i64 %indvar18, 1
  %exitcond29 = icmp eq i64 %indvar.next19, 96
  br i1 %exitcond29, label %for_begin7.preheader, label %for_begin1.preheader, !prof !28

for_end6:                                         ; preds = %if_end.us, %for_body5.preheader
  %indvar.next21 = add nuw nsw i64 %indvar20, 1
  %exitcond28 = icmp eq i64 %indvar.next21, 30
  br i1 %exitcond28, label %for_end3, label %for_begin4.preheader, !prof !28

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %indvar = phi i64 [ 0, %for_begin7.preheader ], [ %indvar.next, %for_end12 ]
  %26 = mul nuw nsw i64 %indvar, 676
  %27 = trunc i64 %indvar to i32
  %28 = lshr i32 %27, 7
  %29 = mul nsw i32 %28, 43200
  %30 = mul nuw nsw i64 %indvar, 1200
  %31 = zext i32 %29 to i64
  br label %for_begin13.preheader

for_end9:                                         ; preds = %for_end12
  ret void

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvar11 = phi i64 [ 0, %for_begin10.preheader ], [ %35, %for_end15 ]
  %32 = mul nuw nsw i64 %indvar11, 26
  %33 = add nuw nsw i64 %32, %26
  %34 = mul nuw nsw i64 %indvar11, 30
  %35 = add nuw nsw i64 %indvar11, 1
  %36 = mul nuw nsw i64 %35, 30
  %37 = mul i64 %indvar11, 30
  %38 = add i64 %37, 60
  %39 = mul i64 %indvar11, 30
  %40 = add i64 %39, 90
  %41 = mul i64 %indvar11, 30
  %42 = add i64 %41, 120
  br label %for_body14

for_end12:                                        ; preds = %for_end15
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond14 = icmp eq i64 %indvar.next, 256
  br i1 %exitcond14, label %for_end9, label %for_begin10.preheader, !prof !28

for_body14:                                       ; preds = %for_end18, %for_begin13.preheader
  %indvars.iv8 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next9, %for_end18 ]
  %43 = add nuw nsw i64 %33, %indvars.iv8
  %44 = getelementptr inbounds float, float* %8, i64 %43
  store float 0.000000e+00, float* %44, align 4, !tbaa !73
  %45 = add nuw nsw i64 %indvars.iv8, %31
  br label %for_begin19.preheader

for_end15:                                        ; preds = %for_end18
  %exitcond13 = icmp eq i64 %35, 26
  br i1 %exitcond13, label %for_end12, label %for_begin13.preheader, !prof !28

for_begin19.preheader:                            ; preds = %for_begin19.preheader, %for_body14
  %indvars.iv = phi i64 [ 0, %for_body14 ], [ %indvars.iv.next, %for_begin19.preheader ]
  %.lcssa.lcssa4 = phi float [ 0.000000e+00, %for_body14 ], [ %223, %for_begin19.preheader ]
  %46 = mul nuw nsw i64 %indvars.iv, 900
  %47 = add nuw nsw i64 %45, %46
  %48 = mul nuw nsw i64 %indvars.iv, 25
  %49 = add nuw nsw i64 %48, %30
  %50 = add nuw nsw i64 %47, %34
  %51 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %50
  %52 = load float, float* %51, align 4, !tbaa !70
  %53 = getelementptr inbounds float, float* %9, i64 %49
  %54 = load float, float* %53, align 4, !tbaa !76
  %55 = tail call float @llvm.fmuladd.f32(float %52, float %54, float %.lcssa.lcssa4)
  %56 = add nuw nsw i64 %50, 1
  %57 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %56
  %58 = load float, float* %57, align 4, !tbaa !70
  %59 = add nuw nsw i64 %49, 1
  %60 = getelementptr inbounds float, float* %9, i64 %59
  %61 = load float, float* %60, align 4, !tbaa !76
  %62 = tail call float @llvm.fmuladd.f32(float %58, float %61, float %55)
  %63 = add nuw nsw i64 %50, 2
  %64 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %63
  %65 = load float, float* %64, align 4, !tbaa !70
  %66 = add nuw nsw i64 %49, 2
  %67 = getelementptr inbounds float, float* %9, i64 %66
  %68 = load float, float* %67, align 4, !tbaa !76
  %69 = tail call float @llvm.fmuladd.f32(float %65, float %68, float %62)
  %70 = add nuw nsw i64 %50, 3
  %71 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %70
  %72 = load float, float* %71, align 4, !tbaa !70
  %73 = add nuw nsw i64 %49, 3
  %74 = getelementptr inbounds float, float* %9, i64 %73
  %75 = load float, float* %74, align 4, !tbaa !76
  %76 = tail call float @llvm.fmuladd.f32(float %72, float %75, float %69)
  %77 = add nuw nsw i64 %50, 4
  %78 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %77
  %79 = load float, float* %78, align 4, !tbaa !70
  %80 = add nuw nsw i64 %49, 4
  %81 = getelementptr inbounds float, float* %9, i64 %80
  %82 = load float, float* %81, align 4, !tbaa !76
  %83 = tail call float @llvm.fmuladd.f32(float %79, float %82, float %76)
  %84 = add nuw nsw i64 %47, %36
  %85 = add nuw nsw i64 %49, 5
  %86 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %84
  %87 = load float, float* %86, align 4, !tbaa !70
  %88 = getelementptr inbounds float, float* %9, i64 %85
  %89 = load float, float* %88, align 4, !tbaa !76
  %90 = tail call float @llvm.fmuladd.f32(float %87, float %89, float %83)
  %91 = add nuw nsw i64 %84, 1
  %92 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %91
  %93 = load float, float* %92, align 4, !tbaa !70
  %94 = add nuw nsw i64 %49, 6
  %95 = getelementptr inbounds float, float* %9, i64 %94
  %96 = load float, float* %95, align 4, !tbaa !76
  %97 = tail call float @llvm.fmuladd.f32(float %93, float %96, float %90)
  %98 = add nuw nsw i64 %84, 2
  %99 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %98
  %100 = load float, float* %99, align 4, !tbaa !70
  %101 = add nuw nsw i64 %49, 7
  %102 = getelementptr inbounds float, float* %9, i64 %101
  %103 = load float, float* %102, align 4, !tbaa !76
  %104 = tail call float @llvm.fmuladd.f32(float %100, float %103, float %97)
  %105 = add nuw nsw i64 %84, 3
  %106 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %105
  %107 = load float, float* %106, align 4, !tbaa !70
  %108 = add nuw nsw i64 %49, 8
  %109 = getelementptr inbounds float, float* %9, i64 %108
  %110 = load float, float* %109, align 4, !tbaa !76
  %111 = tail call float @llvm.fmuladd.f32(float %107, float %110, float %104)
  %112 = add nuw nsw i64 %84, 4
  %113 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %112
  %114 = load float, float* %113, align 4, !tbaa !70
  %115 = add nuw nsw i64 %49, 9
  %116 = getelementptr inbounds float, float* %9, i64 %115
  %117 = load float, float* %116, align 4, !tbaa !76
  %118 = tail call float @llvm.fmuladd.f32(float %114, float %117, float %111)
  %119 = add nuw nsw i64 %47, %38
  %120 = add nuw nsw i64 %49, 10
  %121 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %119
  %122 = load float, float* %121, align 4, !tbaa !70
  %123 = getelementptr inbounds float, float* %9, i64 %120
  %124 = load float, float* %123, align 4, !tbaa !76
  %125 = tail call float @llvm.fmuladd.f32(float %122, float %124, float %118)
  %126 = add nuw nsw i64 %119, 1
  %127 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %126
  %128 = load float, float* %127, align 4, !tbaa !70
  %129 = add nuw nsw i64 %49, 11
  %130 = getelementptr inbounds float, float* %9, i64 %129
  %131 = load float, float* %130, align 4, !tbaa !76
  %132 = tail call float @llvm.fmuladd.f32(float %128, float %131, float %125)
  %133 = add nuw nsw i64 %119, 2
  %134 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %133
  %135 = load float, float* %134, align 4, !tbaa !70
  %136 = add nuw nsw i64 %49, 12
  %137 = getelementptr inbounds float, float* %9, i64 %136
  %138 = load float, float* %137, align 4, !tbaa !76
  %139 = tail call float @llvm.fmuladd.f32(float %135, float %138, float %132)
  %140 = add nuw nsw i64 %119, 3
  %141 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %140
  %142 = load float, float* %141, align 4, !tbaa !70
  %143 = add nuw nsw i64 %49, 13
  %144 = getelementptr inbounds float, float* %9, i64 %143
  %145 = load float, float* %144, align 4, !tbaa !76
  %146 = tail call float @llvm.fmuladd.f32(float %142, float %145, float %139)
  %147 = add nuw nsw i64 %119, 4
  %148 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %147
  %149 = load float, float* %148, align 4, !tbaa !70
  %150 = add nuw nsw i64 %49, 14
  %151 = getelementptr inbounds float, float* %9, i64 %150
  %152 = load float, float* %151, align 4, !tbaa !76
  %153 = tail call float @llvm.fmuladd.f32(float %149, float %152, float %146)
  %154 = add nuw nsw i64 %47, %40
  %155 = add nuw nsw i64 %49, 15
  %156 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %154
  %157 = load float, float* %156, align 4, !tbaa !70
  %158 = getelementptr inbounds float, float* %9, i64 %155
  %159 = load float, float* %158, align 4, !tbaa !76
  %160 = tail call float @llvm.fmuladd.f32(float %157, float %159, float %153)
  %161 = add nuw nsw i64 %154, 1
  %162 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %161
  %163 = load float, float* %162, align 4, !tbaa !70
  %164 = add nuw nsw i64 %49, 16
  %165 = getelementptr inbounds float, float* %9, i64 %164
  %166 = load float, float* %165, align 4, !tbaa !76
  %167 = tail call float @llvm.fmuladd.f32(float %163, float %166, float %160)
  %168 = add nuw nsw i64 %154, 2
  %169 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %168
  %170 = load float, float* %169, align 4, !tbaa !70
  %171 = add nuw nsw i64 %49, 17
  %172 = getelementptr inbounds float, float* %9, i64 %171
  %173 = load float, float* %172, align 4, !tbaa !76
  %174 = tail call float @llvm.fmuladd.f32(float %170, float %173, float %167)
  %175 = add nuw nsw i64 %154, 3
  %176 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %175
  %177 = load float, float* %176, align 4, !tbaa !70
  %178 = add nuw nsw i64 %49, 18
  %179 = getelementptr inbounds float, float* %9, i64 %178
  %180 = load float, float* %179, align 4, !tbaa !76
  %181 = tail call float @llvm.fmuladd.f32(float %177, float %180, float %174)
  %182 = add nuw nsw i64 %154, 4
  %183 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %182
  %184 = load float, float* %183, align 4, !tbaa !70
  %185 = add nuw nsw i64 %49, 19
  %186 = getelementptr inbounds float, float* %9, i64 %185
  %187 = load float, float* %186, align 4, !tbaa !76
  %188 = tail call float @llvm.fmuladd.f32(float %184, float %187, float %181)
  %189 = add nuw nsw i64 %47, %42
  %190 = add nuw nsw i64 %49, 20
  %191 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %189
  %192 = load float, float* %191, align 4, !tbaa !70
  %193 = getelementptr inbounds float, float* %9, i64 %190
  %194 = load float, float* %193, align 4, !tbaa !76
  %195 = tail call float @llvm.fmuladd.f32(float %192, float %194, float %188)
  %196 = add nuw nsw i64 %189, 1
  %197 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %196
  %198 = load float, float* %197, align 4, !tbaa !70
  %199 = add nuw nsw i64 %49, 21
  %200 = getelementptr inbounds float, float* %9, i64 %199
  %201 = load float, float* %200, align 4, !tbaa !76
  %202 = tail call float @llvm.fmuladd.f32(float %198, float %201, float %195)
  %203 = add nuw nsw i64 %189, 2
  %204 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %203
  %205 = load float, float* %204, align 4, !tbaa !70
  %206 = add nuw nsw i64 %49, 22
  %207 = getelementptr inbounds float, float* %9, i64 %206
  %208 = load float, float* %207, align 4, !tbaa !76
  %209 = tail call float @llvm.fmuladd.f32(float %205, float %208, float %202)
  %210 = add nuw nsw i64 %189, 3
  %211 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %210
  %212 = load float, float* %211, align 4, !tbaa !70
  %213 = add nuw nsw i64 %49, 23
  %214 = getelementptr inbounds float, float* %9, i64 %213
  %215 = load float, float* %214, align 4, !tbaa !76
  %216 = tail call float @llvm.fmuladd.f32(float %212, float %215, float %209)
  %217 = add nuw nsw i64 %189, 4
  %218 = getelementptr inbounds [86400 x float], [86400 x float]* %3, i64 0, i64 %217
  %219 = load float, float* %218, align 4, !tbaa !70
  %220 = add nuw nsw i64 %49, 24
  %221 = getelementptr inbounds float, float* %9, i64 %220
  %222 = load float, float* %221, align 4, !tbaa !76
  %223 = tail call float @llvm.fmuladd.f32(float %219, float %222, float %216)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 48
  br i1 %exitcond, label %for_end18, label %for_begin19.preheader, !prof !28

for_end18:                                        ; preds = %for_begin19.preheader
  store float %223, float* %44, align 4, !tbaa !73
  %indvars.iv.next9 = add nuw nsw i64 %indvars.iv8, 1
  %exitcond10 = icmp eq i64 %indvars.iv.next9, 26
  br i1 %exitcond10, label %for_end15, label %for_body14, !prof !28
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
  %4 = mul nuw nsw i64 %indvars.iv6, 676
  %5 = mul nuw nsw i64 %indvars.iv6, 2916
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv3 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next4, %for_begin4.preheader ]
  %6 = mul nuw nsw i64 %indvars.iv3, 26
  %7 = add nuw nsw i64 %6, %4
  %8 = mul nuw nsw i64 %indvars.iv3, 108
  %9 = add nuw nsw i64 %8, %5
  %10 = getelementptr inbounds float, float* %2, i64 %7
  %11 = getelementptr inbounds float, float* %3, i64 %9
  %12 = bitcast float* %11 to <8 x float>*
  %wide.vec = load <8 x float>, <8 x float>* %12, align 4, !tbaa !79
  %strided.vec = shufflevector <8 x float> %wide.vec, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %13 = fcmp olt <4 x float> %strided.vec, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %14 = select <4 x i1> %13, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec
  %15 = or i64 %9, 1
  %16 = getelementptr inbounds float, float* %3, i64 %15
  %17 = bitcast float* %16 to <8 x float>*
  %wide.vec9 = load <8 x float>, <8 x float>* %17, align 4, !tbaa !79
  %strided.vec10 = shufflevector <8 x float> %wide.vec9, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec11 = shufflevector <8 x float> %wide.vec9, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %18 = fcmp ogt <4 x float> %14, %strided.vec10
  %19 = select <4 x i1> %18, <4 x float> %14, <4 x float> %strided.vec10
  %20 = fcmp ogt <4 x float> %19, %strided.vec11
  %21 = select <4 x i1> %20, <4 x float> %19, <4 x float> %strided.vec11
  %22 = add nuw nsw i64 %9, 54
  %23 = getelementptr inbounds float, float* %3, i64 %22
  %24 = bitcast float* %23 to <8 x float>*
  %wide.vec12 = load <8 x float>, <8 x float>* %24, align 4, !tbaa !79
  %strided.vec13 = shufflevector <8 x float> %wide.vec12, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %25 = fcmp ogt <4 x float> %21, %strided.vec13
  %26 = select <4 x i1> %25, <4 x float> %21, <4 x float> %strided.vec13
  %27 = add nuw nsw i64 %9, 55
  %28 = getelementptr inbounds float, float* %3, i64 %27
  %29 = bitcast float* %28 to <8 x float>*
  %wide.vec14 = load <8 x float>, <8 x float>* %29, align 4, !tbaa !79
  %strided.vec15 = shufflevector <8 x float> %wide.vec14, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec16 = shufflevector <8 x float> %wide.vec14, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %30 = fcmp ogt <4 x float> %26, %strided.vec15
  %31 = select <4 x i1> %30, <4 x float> %26, <4 x float> %strided.vec15
  %32 = fcmp ogt <4 x float> %31, %strided.vec16
  %33 = select <4 x i1> %32, <4 x float> %31, <4 x float> %strided.vec16
  %34 = add nuw nsw i64 %9, 108
  %35 = getelementptr inbounds float, float* %3, i64 %34
  %36 = bitcast float* %35 to <8 x float>*
  %wide.vec17 = load <8 x float>, <8 x float>* %36, align 4, !tbaa !79
  %strided.vec18 = shufflevector <8 x float> %wide.vec17, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %37 = fcmp ogt <4 x float> %33, %strided.vec18
  %38 = select <4 x i1> %37, <4 x float> %33, <4 x float> %strided.vec18
  %39 = add nuw nsw i64 %9, 109
  %40 = getelementptr inbounds float, float* %3, i64 %39
  %41 = bitcast float* %40 to <8 x float>*
  %wide.vec19 = load <8 x float>, <8 x float>* %41, align 4, !tbaa !79
  %strided.vec20 = shufflevector <8 x float> %wide.vec19, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec21 = shufflevector <8 x float> %wide.vec19, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %42 = fcmp ogt <4 x float> %38, %strided.vec20
  %43 = select <4 x i1> %42, <4 x float> %38, <4 x float> %strided.vec20
  %44 = fcmp ogt <4 x float> %43, %strided.vec21
  %45 = select <4 x i1> %44, <4 x float> %43, <4 x float> %strided.vec21
  %46 = bitcast float* %10 to <4 x float>*
  store <4 x float> %45, <4 x float>* %46, align 4, !tbaa !82
  %47 = add nuw nsw i64 %7, 4
  %48 = getelementptr inbounds float, float* %2, i64 %47
  %49 = add nuw nsw i64 %9, 8
  %50 = getelementptr inbounds float, float* %3, i64 %49
  %51 = bitcast float* %50 to <8 x float>*
  %wide.vec.1 = load <8 x float>, <8 x float>* %51, align 4, !tbaa !79
  %strided.vec.1 = shufflevector <8 x float> %wide.vec.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %52 = fcmp olt <4 x float> %strided.vec.1, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %53 = select <4 x i1> %52, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.1
  %54 = or i64 %49, 1
  %55 = getelementptr inbounds float, float* %3, i64 %54
  %56 = bitcast float* %55 to <8 x float>*
  %wide.vec9.1 = load <8 x float>, <8 x float>* %56, align 4, !tbaa !79
  %strided.vec10.1 = shufflevector <8 x float> %wide.vec9.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec11.1 = shufflevector <8 x float> %wide.vec9.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %57 = fcmp ogt <4 x float> %53, %strided.vec10.1
  %58 = select <4 x i1> %57, <4 x float> %53, <4 x float> %strided.vec10.1
  %59 = fcmp ogt <4 x float> %58, %strided.vec11.1
  %60 = select <4 x i1> %59, <4 x float> %58, <4 x float> %strided.vec11.1
  %61 = add nuw nsw i64 %9, 62
  %62 = getelementptr inbounds float, float* %3, i64 %61
  %63 = bitcast float* %62 to <8 x float>*
  %wide.vec12.1 = load <8 x float>, <8 x float>* %63, align 4, !tbaa !79
  %strided.vec13.1 = shufflevector <8 x float> %wide.vec12.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %64 = fcmp ogt <4 x float> %60, %strided.vec13.1
  %65 = select <4 x i1> %64, <4 x float> %60, <4 x float> %strided.vec13.1
  %66 = add nuw nsw i64 %9, 63
  %67 = getelementptr inbounds float, float* %3, i64 %66
  %68 = bitcast float* %67 to <8 x float>*
  %wide.vec14.1 = load <8 x float>, <8 x float>* %68, align 4, !tbaa !79
  %strided.vec15.1 = shufflevector <8 x float> %wide.vec14.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec16.1 = shufflevector <8 x float> %wide.vec14.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %69 = fcmp ogt <4 x float> %65, %strided.vec15.1
  %70 = select <4 x i1> %69, <4 x float> %65, <4 x float> %strided.vec15.1
  %71 = fcmp ogt <4 x float> %70, %strided.vec16.1
  %72 = select <4 x i1> %71, <4 x float> %70, <4 x float> %strided.vec16.1
  %73 = add nuw nsw i64 %9, 116
  %74 = getelementptr inbounds float, float* %3, i64 %73
  %75 = bitcast float* %74 to <8 x float>*
  %wide.vec17.1 = load <8 x float>, <8 x float>* %75, align 4, !tbaa !79
  %strided.vec18.1 = shufflevector <8 x float> %wide.vec17.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %76 = fcmp ogt <4 x float> %72, %strided.vec18.1
  %77 = select <4 x i1> %76, <4 x float> %72, <4 x float> %strided.vec18.1
  %78 = add nuw nsw i64 %9, 117
  %79 = getelementptr inbounds float, float* %3, i64 %78
  %80 = bitcast float* %79 to <8 x float>*
  %wide.vec19.1 = load <8 x float>, <8 x float>* %80, align 4, !tbaa !79
  %strided.vec20.1 = shufflevector <8 x float> %wide.vec19.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec21.1 = shufflevector <8 x float> %wide.vec19.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %81 = fcmp ogt <4 x float> %77, %strided.vec20.1
  %82 = select <4 x i1> %81, <4 x float> %77, <4 x float> %strided.vec20.1
  %83 = fcmp ogt <4 x float> %82, %strided.vec21.1
  %84 = select <4 x i1> %83, <4 x float> %82, <4 x float> %strided.vec21.1
  %85 = bitcast float* %48 to <4 x float>*
  store <4 x float> %84, <4 x float>* %85, align 4, !tbaa !82
  %86 = add nuw nsw i64 %7, 8
  %87 = getelementptr inbounds float, float* %2, i64 %86
  %88 = add nuw nsw i64 %9, 16
  %89 = getelementptr inbounds float, float* %3, i64 %88
  %90 = bitcast float* %89 to <8 x float>*
  %wide.vec.2 = load <8 x float>, <8 x float>* %90, align 4, !tbaa !79
  %strided.vec.2 = shufflevector <8 x float> %wide.vec.2, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %91 = fcmp olt <4 x float> %strided.vec.2, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %92 = select <4 x i1> %91, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.2
  %93 = or i64 %88, 1
  %94 = getelementptr inbounds float, float* %3, i64 %93
  %95 = bitcast float* %94 to <8 x float>*
  %wide.vec9.2 = load <8 x float>, <8 x float>* %95, align 4, !tbaa !79
  %strided.vec10.2 = shufflevector <8 x float> %wide.vec9.2, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec11.2 = shufflevector <8 x float> %wide.vec9.2, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %96 = fcmp ogt <4 x float> %92, %strided.vec10.2
  %97 = select <4 x i1> %96, <4 x float> %92, <4 x float> %strided.vec10.2
  %98 = fcmp ogt <4 x float> %97, %strided.vec11.2
  %99 = select <4 x i1> %98, <4 x float> %97, <4 x float> %strided.vec11.2
  %100 = add nuw nsw i64 %9, 70
  %101 = getelementptr inbounds float, float* %3, i64 %100
  %102 = bitcast float* %101 to <8 x float>*
  %wide.vec12.2 = load <8 x float>, <8 x float>* %102, align 4, !tbaa !79
  %strided.vec13.2 = shufflevector <8 x float> %wide.vec12.2, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %103 = fcmp ogt <4 x float> %99, %strided.vec13.2
  %104 = select <4 x i1> %103, <4 x float> %99, <4 x float> %strided.vec13.2
  %105 = add nuw nsw i64 %9, 71
  %106 = getelementptr inbounds float, float* %3, i64 %105
  %107 = bitcast float* %106 to <8 x float>*
  %wide.vec14.2 = load <8 x float>, <8 x float>* %107, align 4, !tbaa !79
  %strided.vec15.2 = shufflevector <8 x float> %wide.vec14.2, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec16.2 = shufflevector <8 x float> %wide.vec14.2, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %108 = fcmp ogt <4 x float> %104, %strided.vec15.2
  %109 = select <4 x i1> %108, <4 x float> %104, <4 x float> %strided.vec15.2
  %110 = fcmp ogt <4 x float> %109, %strided.vec16.2
  %111 = select <4 x i1> %110, <4 x float> %109, <4 x float> %strided.vec16.2
  %112 = add nuw nsw i64 %9, 124
  %113 = getelementptr inbounds float, float* %3, i64 %112
  %114 = bitcast float* %113 to <8 x float>*
  %wide.vec17.2 = load <8 x float>, <8 x float>* %114, align 4, !tbaa !79
  %strided.vec18.2 = shufflevector <8 x float> %wide.vec17.2, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %115 = fcmp ogt <4 x float> %111, %strided.vec18.2
  %116 = select <4 x i1> %115, <4 x float> %111, <4 x float> %strided.vec18.2
  %117 = add nuw nsw i64 %9, 125
  %118 = getelementptr inbounds float, float* %3, i64 %117
  %119 = bitcast float* %118 to <8 x float>*
  %wide.vec19.2 = load <8 x float>, <8 x float>* %119, align 4, !tbaa !79
  %strided.vec20.2 = shufflevector <8 x float> %wide.vec19.2, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec21.2 = shufflevector <8 x float> %wide.vec19.2, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %120 = fcmp ogt <4 x float> %116, %strided.vec20.2
  %121 = select <4 x i1> %120, <4 x float> %116, <4 x float> %strided.vec20.2
  %122 = fcmp ogt <4 x float> %121, %strided.vec21.2
  %123 = select <4 x i1> %122, <4 x float> %121, <4 x float> %strided.vec21.2
  %124 = bitcast float* %87 to <4 x float>*
  store <4 x float> %123, <4 x float>* %124, align 4, !tbaa !82
  %125 = add nuw nsw i64 %7, 12
  %126 = getelementptr inbounds float, float* %2, i64 %125
  %127 = add nuw nsw i64 %9, 24
  %128 = getelementptr inbounds float, float* %3, i64 %127
  %129 = bitcast float* %128 to <8 x float>*
  %wide.vec.3 = load <8 x float>, <8 x float>* %129, align 4, !tbaa !79
  %strided.vec.3 = shufflevector <8 x float> %wide.vec.3, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %130 = fcmp olt <4 x float> %strided.vec.3, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %131 = select <4 x i1> %130, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.3
  %132 = or i64 %127, 1
  %133 = getelementptr inbounds float, float* %3, i64 %132
  %134 = bitcast float* %133 to <8 x float>*
  %wide.vec9.3 = load <8 x float>, <8 x float>* %134, align 4, !tbaa !79
  %strided.vec10.3 = shufflevector <8 x float> %wide.vec9.3, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec11.3 = shufflevector <8 x float> %wide.vec9.3, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %135 = fcmp ogt <4 x float> %131, %strided.vec10.3
  %136 = select <4 x i1> %135, <4 x float> %131, <4 x float> %strided.vec10.3
  %137 = fcmp ogt <4 x float> %136, %strided.vec11.3
  %138 = select <4 x i1> %137, <4 x float> %136, <4 x float> %strided.vec11.3
  %139 = add nuw nsw i64 %9, 78
  %140 = getelementptr inbounds float, float* %3, i64 %139
  %141 = bitcast float* %140 to <8 x float>*
  %wide.vec12.3 = load <8 x float>, <8 x float>* %141, align 4, !tbaa !79
  %strided.vec13.3 = shufflevector <8 x float> %wide.vec12.3, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %142 = fcmp ogt <4 x float> %138, %strided.vec13.3
  %143 = select <4 x i1> %142, <4 x float> %138, <4 x float> %strided.vec13.3
  %144 = add nuw nsw i64 %9, 79
  %145 = getelementptr inbounds float, float* %3, i64 %144
  %146 = bitcast float* %145 to <8 x float>*
  %wide.vec14.3 = load <8 x float>, <8 x float>* %146, align 4, !tbaa !79
  %strided.vec15.3 = shufflevector <8 x float> %wide.vec14.3, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec16.3 = shufflevector <8 x float> %wide.vec14.3, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %147 = fcmp ogt <4 x float> %143, %strided.vec15.3
  %148 = select <4 x i1> %147, <4 x float> %143, <4 x float> %strided.vec15.3
  %149 = fcmp ogt <4 x float> %148, %strided.vec16.3
  %150 = select <4 x i1> %149, <4 x float> %148, <4 x float> %strided.vec16.3
  %151 = add nuw nsw i64 %9, 132
  %152 = getelementptr inbounds float, float* %3, i64 %151
  %153 = bitcast float* %152 to <8 x float>*
  %wide.vec17.3 = load <8 x float>, <8 x float>* %153, align 4, !tbaa !79
  %strided.vec18.3 = shufflevector <8 x float> %wide.vec17.3, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %154 = fcmp ogt <4 x float> %150, %strided.vec18.3
  %155 = select <4 x i1> %154, <4 x float> %150, <4 x float> %strided.vec18.3
  %156 = add nuw nsw i64 %9, 133
  %157 = getelementptr inbounds float, float* %3, i64 %156
  %158 = bitcast float* %157 to <8 x float>*
  %wide.vec19.3 = load <8 x float>, <8 x float>* %158, align 4, !tbaa !79
  %strided.vec20.3 = shufflevector <8 x float> %wide.vec19.3, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec21.3 = shufflevector <8 x float> %wide.vec19.3, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %159 = fcmp ogt <4 x float> %155, %strided.vec20.3
  %160 = select <4 x i1> %159, <4 x float> %155, <4 x float> %strided.vec20.3
  %161 = fcmp ogt <4 x float> %160, %strided.vec21.3
  %162 = select <4 x i1> %161, <4 x float> %160, <4 x float> %strided.vec21.3
  %163 = bitcast float* %126 to <4 x float>*
  store <4 x float> %162, <4 x float>* %163, align 4, !tbaa !82
  %164 = add nuw nsw i64 %7, 16
  %165 = getelementptr inbounds float, float* %2, i64 %164
  %166 = add nuw nsw i64 %9, 32
  %167 = getelementptr inbounds float, float* %3, i64 %166
  %168 = bitcast float* %167 to <8 x float>*
  %wide.vec.4 = load <8 x float>, <8 x float>* %168, align 4, !tbaa !79
  %strided.vec.4 = shufflevector <8 x float> %wide.vec.4, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %169 = fcmp olt <4 x float> %strided.vec.4, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %170 = select <4 x i1> %169, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.4
  %171 = or i64 %166, 1
  %172 = getelementptr inbounds float, float* %3, i64 %171
  %173 = bitcast float* %172 to <8 x float>*
  %wide.vec9.4 = load <8 x float>, <8 x float>* %173, align 4, !tbaa !79
  %strided.vec10.4 = shufflevector <8 x float> %wide.vec9.4, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec11.4 = shufflevector <8 x float> %wide.vec9.4, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %174 = fcmp ogt <4 x float> %170, %strided.vec10.4
  %175 = select <4 x i1> %174, <4 x float> %170, <4 x float> %strided.vec10.4
  %176 = fcmp ogt <4 x float> %175, %strided.vec11.4
  %177 = select <4 x i1> %176, <4 x float> %175, <4 x float> %strided.vec11.4
  %178 = add nuw nsw i64 %9, 86
  %179 = getelementptr inbounds float, float* %3, i64 %178
  %180 = bitcast float* %179 to <8 x float>*
  %wide.vec12.4 = load <8 x float>, <8 x float>* %180, align 4, !tbaa !79
  %strided.vec13.4 = shufflevector <8 x float> %wide.vec12.4, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %181 = fcmp ogt <4 x float> %177, %strided.vec13.4
  %182 = select <4 x i1> %181, <4 x float> %177, <4 x float> %strided.vec13.4
  %183 = add nuw nsw i64 %9, 87
  %184 = getelementptr inbounds float, float* %3, i64 %183
  %185 = bitcast float* %184 to <8 x float>*
  %wide.vec14.4 = load <8 x float>, <8 x float>* %185, align 4, !tbaa !79
  %strided.vec15.4 = shufflevector <8 x float> %wide.vec14.4, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec16.4 = shufflevector <8 x float> %wide.vec14.4, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %186 = fcmp ogt <4 x float> %182, %strided.vec15.4
  %187 = select <4 x i1> %186, <4 x float> %182, <4 x float> %strided.vec15.4
  %188 = fcmp ogt <4 x float> %187, %strided.vec16.4
  %189 = select <4 x i1> %188, <4 x float> %187, <4 x float> %strided.vec16.4
  %190 = add nuw nsw i64 %9, 140
  %191 = getelementptr inbounds float, float* %3, i64 %190
  %192 = bitcast float* %191 to <8 x float>*
  %wide.vec17.4 = load <8 x float>, <8 x float>* %192, align 4, !tbaa !79
  %strided.vec18.4 = shufflevector <8 x float> %wide.vec17.4, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %193 = fcmp ogt <4 x float> %189, %strided.vec18.4
  %194 = select <4 x i1> %193, <4 x float> %189, <4 x float> %strided.vec18.4
  %195 = add nuw nsw i64 %9, 141
  %196 = getelementptr inbounds float, float* %3, i64 %195
  %197 = bitcast float* %196 to <8 x float>*
  %wide.vec19.4 = load <8 x float>, <8 x float>* %197, align 4, !tbaa !79
  %strided.vec20.4 = shufflevector <8 x float> %wide.vec19.4, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec21.4 = shufflevector <8 x float> %wide.vec19.4, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %198 = fcmp ogt <4 x float> %194, %strided.vec20.4
  %199 = select <4 x i1> %198, <4 x float> %194, <4 x float> %strided.vec20.4
  %200 = fcmp ogt <4 x float> %199, %strided.vec21.4
  %201 = select <4 x i1> %200, <4 x float> %199, <4 x float> %strided.vec21.4
  %202 = bitcast float* %165 to <4 x float>*
  store <4 x float> %201, <4 x float>* %202, align 4, !tbaa !82
  %203 = add nuw nsw i64 %7, 20
  %204 = getelementptr inbounds float, float* %2, i64 %203
  %205 = add nuw nsw i64 %9, 40
  %206 = getelementptr inbounds float, float* %3, i64 %205
  %207 = bitcast float* %206 to <8 x float>*
  %wide.vec.5 = load <8 x float>, <8 x float>* %207, align 4, !tbaa !79
  %strided.vec.5 = shufflevector <8 x float> %wide.vec.5, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %208 = fcmp olt <4 x float> %strided.vec.5, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %209 = select <4 x i1> %208, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.5
  %210 = or i64 %205, 1
  %211 = getelementptr inbounds float, float* %3, i64 %210
  %212 = bitcast float* %211 to <8 x float>*
  %wide.vec9.5 = load <8 x float>, <8 x float>* %212, align 4, !tbaa !79
  %strided.vec10.5 = shufflevector <8 x float> %wide.vec9.5, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec11.5 = shufflevector <8 x float> %wide.vec9.5, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %213 = fcmp ogt <4 x float> %209, %strided.vec10.5
  %214 = select <4 x i1> %213, <4 x float> %209, <4 x float> %strided.vec10.5
  %215 = fcmp ogt <4 x float> %214, %strided.vec11.5
  %216 = select <4 x i1> %215, <4 x float> %214, <4 x float> %strided.vec11.5
  %217 = add nuw nsw i64 %9, 94
  %218 = getelementptr inbounds float, float* %3, i64 %217
  %219 = bitcast float* %218 to <8 x float>*
  %wide.vec12.5 = load <8 x float>, <8 x float>* %219, align 4, !tbaa !79
  %strided.vec13.5 = shufflevector <8 x float> %wide.vec12.5, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %220 = fcmp ogt <4 x float> %216, %strided.vec13.5
  %221 = select <4 x i1> %220, <4 x float> %216, <4 x float> %strided.vec13.5
  %222 = add nuw nsw i64 %9, 95
  %223 = getelementptr inbounds float, float* %3, i64 %222
  %224 = bitcast float* %223 to <8 x float>*
  %wide.vec14.5 = load <8 x float>, <8 x float>* %224, align 4, !tbaa !79
  %strided.vec15.5 = shufflevector <8 x float> %wide.vec14.5, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec16.5 = shufflevector <8 x float> %wide.vec14.5, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %225 = fcmp ogt <4 x float> %221, %strided.vec15.5
  %226 = select <4 x i1> %225, <4 x float> %221, <4 x float> %strided.vec15.5
  %227 = fcmp ogt <4 x float> %226, %strided.vec16.5
  %228 = select <4 x i1> %227, <4 x float> %226, <4 x float> %strided.vec16.5
  %229 = add nuw nsw i64 %9, 148
  %230 = getelementptr inbounds float, float* %3, i64 %229
  %231 = bitcast float* %230 to <8 x float>*
  %wide.vec17.5 = load <8 x float>, <8 x float>* %231, align 4, !tbaa !79
  %strided.vec18.5 = shufflevector <8 x float> %wide.vec17.5, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %232 = fcmp ogt <4 x float> %228, %strided.vec18.5
  %233 = select <4 x i1> %232, <4 x float> %228, <4 x float> %strided.vec18.5
  %234 = add nuw nsw i64 %9, 149
  %235 = getelementptr inbounds float, float* %3, i64 %234
  %236 = bitcast float* %235 to <8 x float>*
  %wide.vec19.5 = load <8 x float>, <8 x float>* %236, align 4, !tbaa !79
  %strided.vec20.5 = shufflevector <8 x float> %wide.vec19.5, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec21.5 = shufflevector <8 x float> %wide.vec19.5, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %237 = fcmp ogt <4 x float> %233, %strided.vec20.5
  %238 = select <4 x i1> %237, <4 x float> %233, <4 x float> %strided.vec20.5
  %239 = fcmp ogt <4 x float> %238, %strided.vec21.5
  %240 = select <4 x i1> %239, <4 x float> %238, <4 x float> %strided.vec21.5
  %241 = bitcast float* %204 to <4 x float>*
  store <4 x float> %240, <4 x float>* %241, align 4, !tbaa !82
  %242 = add nuw nsw i64 %7, 24
  %243 = getelementptr inbounds float, float* %2, i64 %242
  %244 = add nuw nsw i64 %9, 48
  %245 = getelementptr inbounds float, float* %3, i64 %244
  %246 = load float, float* %245, align 4, !tbaa !79
  %247 = fcmp olt float %246, 0xC7EFFFFFE0000000
  %248 = select i1 %247, float 0xC7EFFFFFE0000000, float %246
  %249 = or i64 %244, 1
  %250 = getelementptr inbounds float, float* %3, i64 %249
  %251 = load float, float* %250, align 4, !tbaa !79
  %252 = fcmp ogt float %248, %251
  %253 = select i1 %252, float %248, float %251
  %254 = add nuw nsw i64 %9, 50
  %255 = getelementptr inbounds float, float* %3, i64 %254
  %256 = load float, float* %255, align 4, !tbaa !79
  %257 = fcmp ogt float %253, %256
  %258 = select i1 %257, float %253, float %256
  %259 = add nuw nsw i64 %9, 102
  %260 = getelementptr inbounds float, float* %3, i64 %259
  %261 = load float, float* %260, align 4, !tbaa !79
  %262 = fcmp ogt float %258, %261
  %263 = select i1 %262, float %258, float %261
  %264 = add nuw nsw i64 %9, 103
  %265 = getelementptr inbounds float, float* %3, i64 %264
  %266 = load float, float* %265, align 4, !tbaa !79
  %267 = fcmp ogt float %263, %266
  %268 = select i1 %267, float %263, float %266
  %269 = add nuw nsw i64 %9, 104
  %270 = getelementptr inbounds float, float* %3, i64 %269
  %271 = load float, float* %270, align 4, !tbaa !79
  %272 = fcmp ogt float %268, %271
  %273 = select i1 %272, float %268, float %271
  %274 = add nuw nsw i64 %9, 156
  %275 = getelementptr inbounds float, float* %3, i64 %274
  %276 = load float, float* %275, align 4, !tbaa !79
  %277 = fcmp ogt float %273, %276
  %278 = select i1 %277, float %273, float %276
  %279 = add nuw nsw i64 %9, 157
  %280 = getelementptr inbounds float, float* %3, i64 %279
  %281 = load float, float* %280, align 4, !tbaa !79
  %282 = fcmp ogt float %278, %281
  %283 = select i1 %282, float %278, float %281
  %284 = add nuw nsw i64 %9, 158
  %285 = getelementptr inbounds float, float* %3, i64 %284
  %286 = load float, float* %285, align 4, !tbaa !79
  %287 = fcmp ogt float %283, %286
  %288 = select i1 %287, float %283, float %286
  store float %288, float* %243, align 4, !tbaa !82
  %289 = add nuw nsw i64 %7, 25
  %290 = getelementptr inbounds float, float* %2, i64 %289
  %291 = add nuw nsw i64 %9, 50
  %292 = getelementptr inbounds float, float* %3, i64 %291
  %293 = load float, float* %292, align 4, !tbaa !79
  %294 = fcmp olt float %293, 0xC7EFFFFFE0000000
  %295 = select i1 %294, float 0xC7EFFFFFE0000000, float %293
  %296 = or i64 %291, 1
  %297 = getelementptr inbounds float, float* %3, i64 %296
  %298 = load float, float* %297, align 4, !tbaa !79
  %299 = fcmp ogt float %295, %298
  %300 = select i1 %299, float %295, float %298
  %301 = add nuw nsw i64 %9, 52
  %302 = getelementptr inbounds float, float* %3, i64 %301
  %303 = load float, float* %302, align 4, !tbaa !79
  %304 = fcmp ogt float %300, %303
  %305 = select i1 %304, float %300, float %303
  %306 = add nuw nsw i64 %9, 104
  %307 = getelementptr inbounds float, float* %3, i64 %306
  %308 = load float, float* %307, align 4, !tbaa !79
  %309 = fcmp ogt float %305, %308
  %310 = select i1 %309, float %305, float %308
  %311 = add nuw nsw i64 %9, 105
  %312 = getelementptr inbounds float, float* %3, i64 %311
  %313 = load float, float* %312, align 4, !tbaa !79
  %314 = fcmp ogt float %310, %313
  %315 = select i1 %314, float %310, float %313
  %316 = add nuw nsw i64 %9, 106
  %317 = getelementptr inbounds float, float* %3, i64 %316
  %318 = load float, float* %317, align 4, !tbaa !79
  %319 = fcmp ogt float %315, %318
  %320 = select i1 %319, float %315, float %318
  %321 = add nuw nsw i64 %9, 158
  %322 = getelementptr inbounds float, float* %3, i64 %321
  %323 = load float, float* %322, align 4, !tbaa !79
  %324 = fcmp ogt float %320, %323
  %325 = select i1 %324, float %320, float %323
  %326 = add nuw nsw i64 %9, 159
  %327 = getelementptr inbounds float, float* %3, i64 %326
  %328 = load float, float* %327, align 4, !tbaa !79
  %329 = fcmp ogt float %325, %328
  %330 = select i1 %329, float %325, float %328
  %331 = add nuw nsw i64 %9, 160
  %332 = getelementptr inbounds float, float* %3, i64 %331
  %333 = load float, float* %332, align 4, !tbaa !79
  %334 = fcmp ogt float %330, %333
  %335 = select i1 %334, float %330, float %333
  store float %335, float* %290, align 4, !tbaa !82
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 26
  br i1 %exitcond5, label %for_end3, label %for_begin4.preheader, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  %exitcond8 = icmp eq i64 %indvars.iv.next7, 96
  br i1 %exitcond8, label %for_end, label %for_begin1.preheader, !prof !28
}

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
  %6 = mul nuw nsw i64 %indvars.iv1, 676
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
  %21 = mul nuw nsw i64 %indvars.iv, 26
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
  %69 = add nuw nsw i64 %22, 25
  %70 = getelementptr inbounds float, float* %4, i64 %69
  %71 = load float, float* %70, align 4, !tbaa !88
  %72 = fadd float %8, %71
  %73 = getelementptr inbounds float, float* %5, i64 %69
  store float %72, float* %73, align 4, !tbaa !91
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 26
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
  %4 = mul nuw nsw i64 %indvars.iv1, 676
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_begin4.preheader ]
  %5 = mul nuw nsw i64 %indvars.iv, 26
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
  %60 = add nuw nsw i64 %6, 25
  %61 = getelementptr inbounds float, float* %2, i64 %60
  %62 = load float, float* %61, align 4, !tbaa !94
  %63 = fcmp ogt float %62, 0.000000e+00
  %64 = select i1 %63, float %62, float 0.000000e+00
  %65 = getelementptr inbounds float, float* %3, i64 %60
  store float %64, float* %65, align 4, !tbaa !97
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 26
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 256
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !28
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
  %4 = mul nuw nsw i64 %indvars.iv4, 2916
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv1 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next2, %for_begin4.preheader ]
  %5 = mul nuw nsw i64 %indvars.iv1, 54
  %6 = add nuw nsw i64 %5, %4
  %7 = getelementptr inbounds float, float* %2, i64 %6
  %8 = bitcast float* %7 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !100
  %9 = fcmp ogt <4 x float> %wide.load, zeroinitializer
  %10 = select <4 x i1> %9, <4 x float> %wide.load, <4 x float> zeroinitializer
  %11 = getelementptr inbounds float, float* %3, i64 %6
  %12 = bitcast float* %11 to <4 x float>*
  store <4 x float> %10, <4 x float>* %12, align 4, !tbaa !103
  %13 = add nuw nsw i64 %6, 4
  %14 = getelementptr inbounds float, float* %2, i64 %13
  %15 = bitcast float* %14 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %15, align 4, !tbaa !100
  %16 = fcmp ogt <4 x float> %wide.load.1, zeroinitializer
  %17 = select <4 x i1> %16, <4 x float> %wide.load.1, <4 x float> zeroinitializer
  %18 = getelementptr inbounds float, float* %3, i64 %13
  %19 = bitcast float* %18 to <4 x float>*
  store <4 x float> %17, <4 x float>* %19, align 4, !tbaa !103
  %20 = add nuw nsw i64 %6, 8
  %21 = getelementptr inbounds float, float* %2, i64 %20
  %22 = bitcast float* %21 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %22, align 4, !tbaa !100
  %23 = fcmp ogt <4 x float> %wide.load.2, zeroinitializer
  %24 = select <4 x i1> %23, <4 x float> %wide.load.2, <4 x float> zeroinitializer
  %25 = getelementptr inbounds float, float* %3, i64 %20
  %26 = bitcast float* %25 to <4 x float>*
  store <4 x float> %24, <4 x float>* %26, align 4, !tbaa !103
  %27 = add nuw nsw i64 %6, 12
  %28 = getelementptr inbounds float, float* %2, i64 %27
  %29 = bitcast float* %28 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %29, align 4, !tbaa !100
  %30 = fcmp ogt <4 x float> %wide.load.3, zeroinitializer
  %31 = select <4 x i1> %30, <4 x float> %wide.load.3, <4 x float> zeroinitializer
  %32 = getelementptr inbounds float, float* %3, i64 %27
  %33 = bitcast float* %32 to <4 x float>*
  store <4 x float> %31, <4 x float>* %33, align 4, !tbaa !103
  %34 = add nuw nsw i64 %6, 16
  %35 = getelementptr inbounds float, float* %2, i64 %34
  %36 = bitcast float* %35 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %36, align 4, !tbaa !100
  %37 = fcmp ogt <4 x float> %wide.load.4, zeroinitializer
  %38 = select <4 x i1> %37, <4 x float> %wide.load.4, <4 x float> zeroinitializer
  %39 = getelementptr inbounds float, float* %3, i64 %34
  %40 = bitcast float* %39 to <4 x float>*
  store <4 x float> %38, <4 x float>* %40, align 4, !tbaa !103
  %41 = add nuw nsw i64 %6, 20
  %42 = getelementptr inbounds float, float* %2, i64 %41
  %43 = bitcast float* %42 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %43, align 4, !tbaa !100
  %44 = fcmp ogt <4 x float> %wide.load.5, zeroinitializer
  %45 = select <4 x i1> %44, <4 x float> %wide.load.5, <4 x float> zeroinitializer
  %46 = getelementptr inbounds float, float* %3, i64 %41
  %47 = bitcast float* %46 to <4 x float>*
  store <4 x float> %45, <4 x float>* %47, align 4, !tbaa !103
  %48 = add nuw nsw i64 %6, 24
  %49 = getelementptr inbounds float, float* %2, i64 %48
  %50 = bitcast float* %49 to <4 x float>*
  %wide.load.6 = load <4 x float>, <4 x float>* %50, align 4, !tbaa !100
  %51 = fcmp ogt <4 x float> %wide.load.6, zeroinitializer
  %52 = select <4 x i1> %51, <4 x float> %wide.load.6, <4 x float> zeroinitializer
  %53 = getelementptr inbounds float, float* %3, i64 %48
  %54 = bitcast float* %53 to <4 x float>*
  store <4 x float> %52, <4 x float>* %54, align 4, !tbaa !103
  %55 = add nuw nsw i64 %6, 28
  %56 = getelementptr inbounds float, float* %2, i64 %55
  %57 = bitcast float* %56 to <4 x float>*
  %wide.load.7 = load <4 x float>, <4 x float>* %57, align 4, !tbaa !100
  %58 = fcmp ogt <4 x float> %wide.load.7, zeroinitializer
  %59 = select <4 x i1> %58, <4 x float> %wide.load.7, <4 x float> zeroinitializer
  %60 = getelementptr inbounds float, float* %3, i64 %55
  %61 = bitcast float* %60 to <4 x float>*
  store <4 x float> %59, <4 x float>* %61, align 4, !tbaa !103
  %62 = add nuw nsw i64 %6, 32
  %63 = getelementptr inbounds float, float* %2, i64 %62
  %64 = bitcast float* %63 to <4 x float>*
  %wide.load.8 = load <4 x float>, <4 x float>* %64, align 4, !tbaa !100
  %65 = fcmp ogt <4 x float> %wide.load.8, zeroinitializer
  %66 = select <4 x i1> %65, <4 x float> %wide.load.8, <4 x float> zeroinitializer
  %67 = getelementptr inbounds float, float* %3, i64 %62
  %68 = bitcast float* %67 to <4 x float>*
  store <4 x float> %66, <4 x float>* %68, align 4, !tbaa !103
  %69 = add nuw nsw i64 %6, 36
  %70 = getelementptr inbounds float, float* %2, i64 %69
  %71 = bitcast float* %70 to <4 x float>*
  %wide.load.9 = load <4 x float>, <4 x float>* %71, align 4, !tbaa !100
  %72 = fcmp ogt <4 x float> %wide.load.9, zeroinitializer
  %73 = select <4 x i1> %72, <4 x float> %wide.load.9, <4 x float> zeroinitializer
  %74 = getelementptr inbounds float, float* %3, i64 %69
  %75 = bitcast float* %74 to <4 x float>*
  store <4 x float> %73, <4 x float>* %75, align 4, !tbaa !103
  %76 = add nuw nsw i64 %6, 40
  %77 = getelementptr inbounds float, float* %2, i64 %76
  %78 = bitcast float* %77 to <4 x float>*
  %wide.load.10 = load <4 x float>, <4 x float>* %78, align 4, !tbaa !100
  %79 = fcmp ogt <4 x float> %wide.load.10, zeroinitializer
  %80 = select <4 x i1> %79, <4 x float> %wide.load.10, <4 x float> zeroinitializer
  %81 = getelementptr inbounds float, float* %3, i64 %76
  %82 = bitcast float* %81 to <4 x float>*
  store <4 x float> %80, <4 x float>* %82, align 4, !tbaa !103
  %83 = add nuw nsw i64 %6, 44
  %84 = getelementptr inbounds float, float* %2, i64 %83
  %85 = bitcast float* %84 to <4 x float>*
  %wide.load.11 = load <4 x float>, <4 x float>* %85, align 4, !tbaa !100
  %86 = fcmp ogt <4 x float> %wide.load.11, zeroinitializer
  %87 = select <4 x i1> %86, <4 x float> %wide.load.11, <4 x float> zeroinitializer
  %88 = getelementptr inbounds float, float* %3, i64 %83
  %89 = bitcast float* %88 to <4 x float>*
  store <4 x float> %87, <4 x float>* %89, align 4, !tbaa !103
  %90 = add nuw nsw i64 %6, 48
  %91 = getelementptr inbounds float, float* %2, i64 %90
  %92 = bitcast float* %91 to <4 x float>*
  %wide.load.12 = load <4 x float>, <4 x float>* %92, align 4, !tbaa !100
  %93 = fcmp ogt <4 x float> %wide.load.12, zeroinitializer
  %94 = select <4 x i1> %93, <4 x float> %wide.load.12, <4 x float> zeroinitializer
  %95 = getelementptr inbounds float, float* %3, i64 %90
  %96 = bitcast float* %95 to <4 x float>*
  store <4 x float> %94, <4 x float>* %96, align 4, !tbaa !103
  %97 = add nuw nsw i64 %6, 52
  %98 = getelementptr inbounds float, float* %2, i64 %97
  %99 = load float, float* %98, align 4, !tbaa !100
  %100 = fcmp ogt float %99, 0.000000e+00
  %101 = select i1 %100, float %99, float 0.000000e+00
  %102 = getelementptr inbounds float, float* %3, i64 %97
  store float %101, float* %102, align 4, !tbaa !103
  %103 = add nuw nsw i64 %6, 53
  %104 = getelementptr inbounds float, float* %2, i64 %103
  %105 = load float, float* %104, align 4, !tbaa !100
  %106 = fcmp ogt float %105, 0.000000e+00
  %107 = select i1 %106, float %105, float 0.000000e+00
  %108 = getelementptr inbounds float, float* %3, i64 %103
  store float %107, float* %108, align 4, !tbaa !103
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 54
  br i1 %exitcond3, label %for_end3, label %for_begin4.preheader, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 96
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !28
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
  %2 = alloca [173056 x float], align 16
  %3 = alloca [175760 x float], align 16
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar34 = phi i64 [ 0, %entry ], [ %indvar.next35, %for_end3 ]
  %4 = phi i32 [ 0, %entry ], [ %59, %for_end3 ]
  %5 = mul nuw nsw i64 %indvar34, 676
  %6 = mul nuw nsw i64 %indvar34, 2704
  %scevgep40 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %5
  %scevgep4041 = bitcast float* %scevgep40 to i8*
  %.off = add nsw i32 %4, -2
  %7 = icmp ult i32 %.off, 256
  br i1 %7, label %for_begin4.preheader.us.preheader, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep4041, i8 0, i64 2704, i1 false)
  br label %for_end3

for_begin4.preheader.us.preheader:                ; preds = %for_begin1.preheader
  %8 = add nsw i64 %6, -5408
  %scevgep48 = getelementptr i8, i8* %0, i64 %8
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep4041, i8* align 4 %scevgep48, i64 104, i1 false)
  %9 = add nuw nsw i64 %5, 26
  %scevgep.1 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %9
  %scevgep47.1 = bitcast float* %scevgep.1 to i8*
  %10 = add nsw i64 %6, -5304
  %scevgep48.1 = getelementptr i8, i8* %0, i64 %10
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.1, i8* align 4 %scevgep48.1, i64 104, i1 false)
  %11 = add nuw nsw i64 %5, 52
  %scevgep.2 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %11
  %scevgep47.2 = bitcast float* %scevgep.2 to i8*
  %12 = add nsw i64 %6, -5200
  %scevgep48.2 = getelementptr i8, i8* %0, i64 %12
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep47.2, i8* align 4 %scevgep48.2, i64 104, i1 false)
  %13 = add nuw nsw i64 %5, 78
  %scevgep.3 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %13
  %scevgep47.3 = bitcast float* %scevgep.3 to i8*
  %14 = add nsw i64 %6, -5096
  %scevgep48.3 = getelementptr i8, i8* %0, i64 %14
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.3, i8* align 4 %scevgep48.3, i64 104, i1 false)
  %15 = add nuw nsw i64 %5, 104
  %scevgep.4 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %15
  %scevgep47.4 = bitcast float* %scevgep.4 to i8*
  %16 = add nsw i64 %6, -4992
  %scevgep48.4 = getelementptr i8, i8* %0, i64 %16
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep47.4, i8* align 4 %scevgep48.4, i64 104, i1 false)
  %17 = add nuw nsw i64 %5, 130
  %scevgep.5 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %17
  %scevgep47.5 = bitcast float* %scevgep.5 to i8*
  %18 = add nsw i64 %6, -4888
  %scevgep48.5 = getelementptr i8, i8* %0, i64 %18
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.5, i8* align 4 %scevgep48.5, i64 104, i1 false)
  %19 = add nuw nsw i64 %5, 156
  %scevgep.6 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %19
  %scevgep47.6 = bitcast float* %scevgep.6 to i8*
  %20 = add nsw i64 %6, -4784
  %scevgep48.6 = getelementptr i8, i8* %0, i64 %20
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep47.6, i8* align 4 %scevgep48.6, i64 104, i1 false)
  %21 = add nuw nsw i64 %5, 182
  %scevgep.7 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %21
  %scevgep47.7 = bitcast float* %scevgep.7 to i8*
  %22 = add nsw i64 %6, -4680
  %scevgep48.7 = getelementptr i8, i8* %0, i64 %22
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.7, i8* align 4 %scevgep48.7, i64 104, i1 false)
  %23 = add nuw nsw i64 %5, 208
  %scevgep.8 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %23
  %scevgep47.8 = bitcast float* %scevgep.8 to i8*
  %24 = add nsw i64 %6, -4576
  %scevgep48.8 = getelementptr i8, i8* %0, i64 %24
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep47.8, i8* align 4 %scevgep48.8, i64 104, i1 false)
  %25 = add nuw nsw i64 %5, 234
  %scevgep.9 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %25
  %scevgep47.9 = bitcast float* %scevgep.9 to i8*
  %26 = add nsw i64 %6, -4472
  %scevgep48.9 = getelementptr i8, i8* %0, i64 %26
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.9, i8* align 4 %scevgep48.9, i64 104, i1 false)
  %27 = add nuw nsw i64 %5, 260
  %scevgep.10 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %27
  %scevgep47.10 = bitcast float* %scevgep.10 to i8*
  %28 = add nsw i64 %6, -4368
  %scevgep48.10 = getelementptr i8, i8* %0, i64 %28
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep47.10, i8* align 4 %scevgep48.10, i64 104, i1 false)
  %29 = add nuw nsw i64 %5, 286
  %scevgep.11 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %29
  %scevgep47.11 = bitcast float* %scevgep.11 to i8*
  %30 = add nsw i64 %6, -4264
  %scevgep48.11 = getelementptr i8, i8* %0, i64 %30
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.11, i8* align 4 %scevgep48.11, i64 104, i1 false)
  %31 = add nuw nsw i64 %5, 312
  %scevgep.12 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %31
  %scevgep47.12 = bitcast float* %scevgep.12 to i8*
  %32 = add nsw i64 %6, -4160
  %scevgep48.12 = getelementptr i8, i8* %0, i64 %32
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep47.12, i8* align 4 %scevgep48.12, i64 104, i1 false)
  %33 = add nuw nsw i64 %5, 338
  %scevgep.13 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %33
  %scevgep47.13 = bitcast float* %scevgep.13 to i8*
  %34 = add nsw i64 %6, -4056
  %scevgep48.13 = getelementptr i8, i8* %0, i64 %34
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.13, i8* align 4 %scevgep48.13, i64 104, i1 false)
  %35 = add nuw nsw i64 %5, 364
  %scevgep.14 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %35
  %scevgep47.14 = bitcast float* %scevgep.14 to i8*
  %36 = add nsw i64 %6, -3952
  %scevgep48.14 = getelementptr i8, i8* %0, i64 %36
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep47.14, i8* align 4 %scevgep48.14, i64 104, i1 false)
  %37 = add nuw nsw i64 %5, 390
  %scevgep.15 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %37
  %scevgep47.15 = bitcast float* %scevgep.15 to i8*
  %38 = add nsw i64 %6, -3848
  %scevgep48.15 = getelementptr i8, i8* %0, i64 %38
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.15, i8* align 4 %scevgep48.15, i64 104, i1 false)
  %39 = add nuw nsw i64 %5, 416
  %scevgep.16 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %39
  %scevgep47.16 = bitcast float* %scevgep.16 to i8*
  %40 = add nsw i64 %6, -3744
  %scevgep48.16 = getelementptr i8, i8* %0, i64 %40
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep47.16, i8* align 4 %scevgep48.16, i64 104, i1 false)
  %41 = add nuw nsw i64 %5, 442
  %scevgep.17 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %41
  %scevgep47.17 = bitcast float* %scevgep.17 to i8*
  %42 = add nsw i64 %6, -3640
  %scevgep48.17 = getelementptr i8, i8* %0, i64 %42
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.17, i8* align 4 %scevgep48.17, i64 104, i1 false)
  %43 = add nuw nsw i64 %5, 468
  %scevgep.18 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %43
  %scevgep47.18 = bitcast float* %scevgep.18 to i8*
  %44 = add nsw i64 %6, -3536
  %scevgep48.18 = getelementptr i8, i8* %0, i64 %44
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep47.18, i8* align 4 %scevgep48.18, i64 104, i1 false)
  %45 = add nuw nsw i64 %5, 494
  %scevgep.19 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %45
  %scevgep47.19 = bitcast float* %scevgep.19 to i8*
  %46 = add nsw i64 %6, -3432
  %scevgep48.19 = getelementptr i8, i8* %0, i64 %46
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.19, i8* align 4 %scevgep48.19, i64 104, i1 false)
  %47 = add nuw nsw i64 %5, 520
  %scevgep.20 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %47
  %scevgep47.20 = bitcast float* %scevgep.20 to i8*
  %48 = add nsw i64 %6, -3328
  %scevgep48.20 = getelementptr i8, i8* %0, i64 %48
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep47.20, i8* align 4 %scevgep48.20, i64 104, i1 false)
  %49 = add nuw nsw i64 %5, 546
  %scevgep.21 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %49
  %scevgep47.21 = bitcast float* %scevgep.21 to i8*
  %50 = add nsw i64 %6, -3224
  %scevgep48.21 = getelementptr i8, i8* %0, i64 %50
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.21, i8* align 4 %scevgep48.21, i64 104, i1 false)
  %51 = add nuw nsw i64 %5, 572
  %scevgep.22 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %51
  %scevgep47.22 = bitcast float* %scevgep.22 to i8*
  %52 = add nsw i64 %6, -3120
  %scevgep48.22 = getelementptr i8, i8* %0, i64 %52
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep47.22, i8* align 4 %scevgep48.22, i64 104, i1 false)
  %53 = add nuw nsw i64 %5, 598
  %scevgep.23 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %53
  %scevgep47.23 = bitcast float* %scevgep.23 to i8*
  %54 = add nsw i64 %6, -3016
  %scevgep48.23 = getelementptr i8, i8* %0, i64 %54
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.23, i8* align 4 %scevgep48.23, i64 104, i1 false)
  %55 = add nuw nsw i64 %5, 624
  %scevgep.24 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %55
  %scevgep47.24 = bitcast float* %scevgep.24 to i8*
  %56 = add nsw i64 %6, -2912
  %scevgep48.24 = getelementptr i8, i8* %0, i64 %56
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep47.24, i8* align 4 %scevgep48.24, i64 104, i1 false)
  %57 = add nuw nsw i64 %5, 650
  %scevgep.25 = getelementptr [175760 x float], [175760 x float]* %3, i64 0, i64 %57
  %scevgep47.25 = bitcast float* %scevgep.25 to i8*
  %58 = add nsw i64 %6, -2808
  %scevgep48.25 = getelementptr i8, i8* %0, i64 %58
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 8 %scevgep47.25, i8* align 4 %scevgep48.25, i64 104, i1 false)
  br label %for_end3

for_end3:                                         ; preds = %for_begin4.preheader.preheader, %for_begin4.preheader.us.preheader
  %59 = add nuw nsw i32 %4, 1
  %indvar.next35 = add nuw nsw i64 %indvar34, 1
  %exitcond50 = icmp eq i64 %indvar.next35, 260
  br i1 %exitcond50, label %for_begin10.preheader.preheader, label %for_begin1.preheader, !prof !28

for_begin10.preheader.preheader:                  ; preds = %for_end3
  %60 = bitcast i8* %0 to float*
  br label %for_begin10.preheader

for_begin10.preheader:                            ; preds = %for_end12, %for_begin10.preheader.preheader
  %indvar = phi i64 [ 0, %for_begin10.preheader.preheader ], [ %indvar.next, %for_end12 ]
  %61 = mul nuw nsw i64 %indvar, 676
  br label %for_begin13.preheader

for_begin13.preheader:                            ; preds = %for_begin13.preheader, %for_begin10.preheader
  %indvar26 = phi i64 [ 0, %for_begin10.preheader ], [ %indvar.next27, %for_begin13.preheader ]
  %62 = mul nuw nsw i64 %indvar26, 26
  %63 = add nuw nsw i64 %62, %61
  %64 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %63
  %65 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %63
  %66 = bitcast float* %65 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %66, align 8, !tbaa !106
  %67 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load, <4 x float> %wide.load, <4 x float> zeroinitializer)
  %68 = add nuw nsw i64 %63, 676
  %69 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %68
  %70 = bitcast float* %69 to <4 x float>*
  %wide.load51 = load <4 x float>, <4 x float>* %70, align 8, !tbaa !106
  %71 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load51, <4 x float> %wide.load51, <4 x float> %67)
  %72 = add nuw nsw i64 %63, 1352
  %73 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %72
  %74 = bitcast float* %73 to <4 x float>*
  %wide.load52 = load <4 x float>, <4 x float>* %74, align 8, !tbaa !106
  %75 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load52, <4 x float> %wide.load52, <4 x float> %71)
  %76 = add nuw nsw i64 %63, 2028
  %77 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %76
  %78 = bitcast float* %77 to <4 x float>*
  %wide.load53 = load <4 x float>, <4 x float>* %78, align 8, !tbaa !106
  %79 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load53, <4 x float> %wide.load53, <4 x float> %75)
  %80 = add nuw nsw i64 %63, 2704
  %81 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %80
  %82 = bitcast float* %81 to <4 x float>*
  %wide.load54 = load <4 x float>, <4 x float>* %82, align 8, !tbaa !106
  %83 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54, <4 x float> %wide.load54, <4 x float> %79)
  %84 = bitcast float* %64 to <4 x float>*
  store <4 x float> %83, <4 x float>* %84, align 8, !tbaa !109
  %85 = add nuw nsw i64 %63, 4
  %86 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %85
  %87 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %85
  %88 = bitcast float* %87 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %88, align 8, !tbaa !106
  %89 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.1, <4 x float> %wide.load.1, <4 x float> zeroinitializer)
  %90 = add nuw nsw i64 %63, 680
  %91 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %90
  %92 = bitcast float* %91 to <4 x float>*
  %wide.load51.1 = load <4 x float>, <4 x float>* %92, align 8, !tbaa !106
  %93 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load51.1, <4 x float> %wide.load51.1, <4 x float> %89)
  %94 = add nuw nsw i64 %63, 1356
  %95 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %94
  %96 = bitcast float* %95 to <4 x float>*
  %wide.load52.1 = load <4 x float>, <4 x float>* %96, align 8, !tbaa !106
  %97 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load52.1, <4 x float> %wide.load52.1, <4 x float> %93)
  %98 = add nuw nsw i64 %63, 2032
  %99 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %98
  %100 = bitcast float* %99 to <4 x float>*
  %wide.load53.1 = load <4 x float>, <4 x float>* %100, align 8, !tbaa !106
  %101 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load53.1, <4 x float> %wide.load53.1, <4 x float> %97)
  %102 = add nuw nsw i64 %63, 2708
  %103 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %102
  %104 = bitcast float* %103 to <4 x float>*
  %wide.load54.1 = load <4 x float>, <4 x float>* %104, align 8, !tbaa !106
  %105 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.1, <4 x float> %wide.load54.1, <4 x float> %101)
  %106 = bitcast float* %86 to <4 x float>*
  store <4 x float> %105, <4 x float>* %106, align 8, !tbaa !109
  %107 = add nuw nsw i64 %63, 8
  %108 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %107
  %109 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %107
  %110 = bitcast float* %109 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %110, align 8, !tbaa !106
  %111 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.2, <4 x float> %wide.load.2, <4 x float> zeroinitializer)
  %112 = add nuw nsw i64 %63, 684
  %113 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %112
  %114 = bitcast float* %113 to <4 x float>*
  %wide.load51.2 = load <4 x float>, <4 x float>* %114, align 8, !tbaa !106
  %115 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load51.2, <4 x float> %wide.load51.2, <4 x float> %111)
  %116 = add nuw nsw i64 %63, 1360
  %117 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %116
  %118 = bitcast float* %117 to <4 x float>*
  %wide.load52.2 = load <4 x float>, <4 x float>* %118, align 8, !tbaa !106
  %119 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load52.2, <4 x float> %wide.load52.2, <4 x float> %115)
  %120 = add nuw nsw i64 %63, 2036
  %121 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %120
  %122 = bitcast float* %121 to <4 x float>*
  %wide.load53.2 = load <4 x float>, <4 x float>* %122, align 8, !tbaa !106
  %123 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load53.2, <4 x float> %wide.load53.2, <4 x float> %119)
  %124 = add nuw nsw i64 %63, 2712
  %125 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %124
  %126 = bitcast float* %125 to <4 x float>*
  %wide.load54.2 = load <4 x float>, <4 x float>* %126, align 8, !tbaa !106
  %127 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.2, <4 x float> %wide.load54.2, <4 x float> %123)
  %128 = bitcast float* %108 to <4 x float>*
  store <4 x float> %127, <4 x float>* %128, align 8, !tbaa !109
  %129 = add nuw nsw i64 %63, 12
  %130 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %129
  %131 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %129
  %132 = bitcast float* %131 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %132, align 8, !tbaa !106
  %133 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.3, <4 x float> %wide.load.3, <4 x float> zeroinitializer)
  %134 = add nuw nsw i64 %63, 688
  %135 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %134
  %136 = bitcast float* %135 to <4 x float>*
  %wide.load51.3 = load <4 x float>, <4 x float>* %136, align 8, !tbaa !106
  %137 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load51.3, <4 x float> %wide.load51.3, <4 x float> %133)
  %138 = add nuw nsw i64 %63, 1364
  %139 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %138
  %140 = bitcast float* %139 to <4 x float>*
  %wide.load52.3 = load <4 x float>, <4 x float>* %140, align 8, !tbaa !106
  %141 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load52.3, <4 x float> %wide.load52.3, <4 x float> %137)
  %142 = add nuw nsw i64 %63, 2040
  %143 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %142
  %144 = bitcast float* %143 to <4 x float>*
  %wide.load53.3 = load <4 x float>, <4 x float>* %144, align 8, !tbaa !106
  %145 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load53.3, <4 x float> %wide.load53.3, <4 x float> %141)
  %146 = add nuw nsw i64 %63, 2716
  %147 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %146
  %148 = bitcast float* %147 to <4 x float>*
  %wide.load54.3 = load <4 x float>, <4 x float>* %148, align 8, !tbaa !106
  %149 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.3, <4 x float> %wide.load54.3, <4 x float> %145)
  %150 = bitcast float* %130 to <4 x float>*
  store <4 x float> %149, <4 x float>* %150, align 8, !tbaa !109
  %151 = add nuw nsw i64 %63, 16
  %152 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %151
  %153 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %151
  %154 = bitcast float* %153 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %154, align 8, !tbaa !106
  %155 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.4, <4 x float> %wide.load.4, <4 x float> zeroinitializer)
  %156 = add nuw nsw i64 %63, 692
  %157 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %156
  %158 = bitcast float* %157 to <4 x float>*
  %wide.load51.4 = load <4 x float>, <4 x float>* %158, align 8, !tbaa !106
  %159 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load51.4, <4 x float> %wide.load51.4, <4 x float> %155)
  %160 = add nuw nsw i64 %63, 1368
  %161 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %160
  %162 = bitcast float* %161 to <4 x float>*
  %wide.load52.4 = load <4 x float>, <4 x float>* %162, align 8, !tbaa !106
  %163 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load52.4, <4 x float> %wide.load52.4, <4 x float> %159)
  %164 = add nuw nsw i64 %63, 2044
  %165 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %164
  %166 = bitcast float* %165 to <4 x float>*
  %wide.load53.4 = load <4 x float>, <4 x float>* %166, align 8, !tbaa !106
  %167 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load53.4, <4 x float> %wide.load53.4, <4 x float> %163)
  %168 = add nuw nsw i64 %63, 2720
  %169 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %168
  %170 = bitcast float* %169 to <4 x float>*
  %wide.load54.4 = load <4 x float>, <4 x float>* %170, align 8, !tbaa !106
  %171 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.4, <4 x float> %wide.load54.4, <4 x float> %167)
  %172 = bitcast float* %152 to <4 x float>*
  store <4 x float> %171, <4 x float>* %172, align 8, !tbaa !109
  %173 = add nuw nsw i64 %63, 20
  %174 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %173
  %175 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %173
  %176 = bitcast float* %175 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %176, align 8, !tbaa !106
  %177 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load.5, <4 x float> %wide.load.5, <4 x float> zeroinitializer)
  %178 = add nuw nsw i64 %63, 696
  %179 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %178
  %180 = bitcast float* %179 to <4 x float>*
  %wide.load51.5 = load <4 x float>, <4 x float>* %180, align 8, !tbaa !106
  %181 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load51.5, <4 x float> %wide.load51.5, <4 x float> %177)
  %182 = add nuw nsw i64 %63, 1372
  %183 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %182
  %184 = bitcast float* %183 to <4 x float>*
  %wide.load52.5 = load <4 x float>, <4 x float>* %184, align 8, !tbaa !106
  %185 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load52.5, <4 x float> %wide.load52.5, <4 x float> %181)
  %186 = add nuw nsw i64 %63, 2048
  %187 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %186
  %188 = bitcast float* %187 to <4 x float>*
  %wide.load53.5 = load <4 x float>, <4 x float>* %188, align 8, !tbaa !106
  %189 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load53.5, <4 x float> %wide.load53.5, <4 x float> %185)
  %190 = add nuw nsw i64 %63, 2724
  %191 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %190
  %192 = bitcast float* %191 to <4 x float>*
  %wide.load54.5 = load <4 x float>, <4 x float>* %192, align 8, !tbaa !106
  %193 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %wide.load54.5, <4 x float> %wide.load54.5, <4 x float> %189)
  %194 = bitcast float* %174 to <4 x float>*
  store <4 x float> %193, <4 x float>* %194, align 8, !tbaa !109
  %195 = add nuw nsw i64 %63, 24
  %196 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %195
  %197 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %195
  %198 = load float, float* %197, align 8, !tbaa !106
  %199 = tail call float @llvm.fmuladd.f32(float %198, float %198, float 0.000000e+00)
  %200 = add nuw nsw i64 %63, 700
  %201 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %200
  %202 = load float, float* %201, align 8, !tbaa !106
  %203 = tail call float @llvm.fmuladd.f32(float %202, float %202, float %199)
  %204 = add nuw nsw i64 %63, 1376
  %205 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %204
  %206 = load float, float* %205, align 8, !tbaa !106
  %207 = tail call float @llvm.fmuladd.f32(float %206, float %206, float %203)
  %208 = add nuw nsw i64 %63, 2052
  %209 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %208
  %210 = load float, float* %209, align 8, !tbaa !106
  %211 = tail call float @llvm.fmuladd.f32(float %210, float %210, float %207)
  %212 = add nuw nsw i64 %63, 2728
  %213 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %212
  %214 = load float, float* %213, align 8, !tbaa !106
  %215 = tail call float @llvm.fmuladd.f32(float %214, float %214, float %211)
  store float %215, float* %196, align 8, !tbaa !109
  %216 = add nuw nsw i64 %63, 25
  %217 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %216
  %218 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %216
  %219 = load float, float* %218, align 4, !tbaa !106
  %220 = tail call float @llvm.fmuladd.f32(float %219, float %219, float 0.000000e+00)
  %221 = add nuw nsw i64 %63, 701
  %222 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %221
  %223 = load float, float* %222, align 4, !tbaa !106
  %224 = tail call float @llvm.fmuladd.f32(float %223, float %223, float %220)
  %225 = add nuw nsw i64 %63, 1377
  %226 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %225
  %227 = load float, float* %226, align 4, !tbaa !106
  %228 = tail call float @llvm.fmuladd.f32(float %227, float %227, float %224)
  %229 = add nuw nsw i64 %63, 2053
  %230 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %229
  %231 = load float, float* %230, align 4, !tbaa !106
  %232 = tail call float @llvm.fmuladd.f32(float %231, float %231, float %228)
  %233 = add nuw nsw i64 %63, 2729
  %234 = getelementptr inbounds [175760 x float], [175760 x float]* %3, i64 0, i64 %233
  %235 = load float, float* %234, align 4, !tbaa !106
  %236 = tail call float @llvm.fmuladd.f32(float %235, float %235, float %232)
  store float %236, float* %217, align 4, !tbaa !109
  %indvar.next27 = add nuw nsw i64 %indvar26, 1
  %exitcond29 = icmp eq i64 %indvar.next27, 26
  br i1 %exitcond29, label %for_end12, label %for_begin13.preheader, !prof !28

for_end12:                                        ; preds = %for_begin13.preheader
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond30 = icmp eq i64 %indvar.next, 256
  br i1 %exitcond30, label %for_begin22.preheader, label %for_begin10.preheader, !prof !28

for_begin22.preheader:                            ; preds = %for_end12, %for_end24
  %indvars.iv17 = phi i64 [ %indvars.iv.next18, %for_end24 ], [ 0, %for_end12 ]
  %237 = mul nuw nsw i64 %indvars.iv17, 676
  br label %for_begin25.preheader

for_begin28.preheader:                            ; preds = %for_end24
  %238 = bitcast i8* %1 to float*
  br label %for_begin31.preheader

for_begin25.preheader:                            ; preds = %for_begin25.preheader, %for_begin22.preheader
  %indvars.iv14 = phi i64 [ 0, %for_begin22.preheader ], [ %indvars.iv.next15, %for_begin25.preheader ]
  %239 = mul nuw nsw i64 %indvars.iv14, 26
  %240 = add nuw nsw i64 %239, %237
  %241 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %240
  %242 = bitcast float* %241 to <4 x float>*
  %243 = load <4 x float>, <4 x float>* %242, align 8, !tbaa !109
  %244 = fmul <4 x float> %243, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %245 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %244, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %246 = call <4 x float> @llvm.pow.v4f32(<4 x float> %245, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %247 = bitcast float* %241 to <4 x float>*
  store <4 x float> %246, <4 x float>* %247, align 8, !tbaa !109
  %248 = add nuw nsw i64 %240, 4
  %249 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %248
  %250 = bitcast float* %249 to <4 x float>*
  %251 = load <4 x float>, <4 x float>* %250, align 8, !tbaa !109
  %252 = fmul <4 x float> %251, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %253 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %252, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %254 = call <4 x float> @llvm.pow.v4f32(<4 x float> %253, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %255 = bitcast float* %249 to <4 x float>*
  store <4 x float> %254, <4 x float>* %255, align 8, !tbaa !109
  %256 = add nuw nsw i64 %240, 8
  %257 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %256
  %258 = bitcast float* %257 to <4 x float>*
  %259 = load <4 x float>, <4 x float>* %258, align 8, !tbaa !109
  %260 = fmul <4 x float> %259, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %261 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %260, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %262 = call <4 x float> @llvm.pow.v4f32(<4 x float> %261, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %263 = bitcast float* %257 to <4 x float>*
  store <4 x float> %262, <4 x float>* %263, align 8, !tbaa !109
  %264 = add nuw nsw i64 %240, 12
  %265 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %264
  %266 = bitcast float* %265 to <4 x float>*
  %267 = load <4 x float>, <4 x float>* %266, align 8, !tbaa !109
  %268 = fmul <4 x float> %267, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %269 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %268, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %270 = call <4 x float> @llvm.pow.v4f32(<4 x float> %269, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %271 = bitcast float* %265 to <4 x float>*
  store <4 x float> %270, <4 x float>* %271, align 8, !tbaa !109
  %272 = add nuw nsw i64 %240, 16
  %273 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %272
  %274 = bitcast float* %273 to <4 x float>*
  %275 = load <4 x float>, <4 x float>* %274, align 8, !tbaa !109
  %276 = fmul <4 x float> %275, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %277 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %276, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %278 = call <4 x float> @llvm.pow.v4f32(<4 x float> %277, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %279 = bitcast float* %273 to <4 x float>*
  store <4 x float> %278, <4 x float>* %279, align 8, !tbaa !109
  %280 = add nuw nsw i64 %240, 20
  %281 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %280
  %282 = bitcast float* %281 to <4 x float>*
  %283 = load <4 x float>, <4 x float>* %282, align 8, !tbaa !109
  %284 = fmul <4 x float> %283, <float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000, float 0x3F1A36E2E0000000>
  %285 = call <4 x float> @llvm.fmuladd.v4f32(<4 x float> %284, <4 x float> <float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000, float 0x3FC99999A0000000>, <4 x float> <float 1.000000e+00, float 1.000000e+00, float 1.000000e+00, float 1.000000e+00>)
  %286 = call <4 x float> @llvm.pow.v4f32(<4 x float> %285, <4 x float> <float 7.500000e-01, float 7.500000e-01, float 7.500000e-01, float 7.500000e-01>)
  %287 = bitcast float* %281 to <4 x float>*
  store <4 x float> %286, <4 x float>* %287, align 8, !tbaa !109
  %288 = add nuw nsw i64 %240, 24
  %289 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %288
  %290 = load float, float* %289, align 8, !tbaa !109
  %291 = fmul float %290, 0x3F1A36E2E0000000
  %292 = tail call float @llvm.fmuladd.f32(float %291, float 0x3FC99999A0000000, float 1.000000e+00)
  %293 = tail call float @llvm.pow.f32(float %292, float 7.500000e-01)
  store float %293, float* %289, align 8, !tbaa !109
  %294 = add nuw nsw i64 %240, 25
  %295 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %294
  %296 = load float, float* %295, align 4, !tbaa !109
  %297 = fmul float %296, 0x3F1A36E2E0000000
  %298 = tail call float @llvm.fmuladd.f32(float %297, float 0x3FC99999A0000000, float 1.000000e+00)
  %299 = tail call float @llvm.pow.f32(float %298, float 7.500000e-01)
  store float %299, float* %295, align 4, !tbaa !109
  %indvars.iv.next15 = add nuw nsw i64 %indvars.iv14, 1
  %exitcond16 = icmp eq i64 %indvars.iv.next15, 26
  br i1 %exitcond16, label %for_end24, label %for_begin25.preheader, !prof !28

for_end24:                                        ; preds = %for_begin25.preheader
  %indvars.iv.next18 = add nuw nsw i64 %indvars.iv17, 1
  %exitcond19 = icmp eq i64 %indvars.iv.next18, 256
  br i1 %exitcond19, label %for_begin28.preheader, label %for_begin22.preheader, !prof !28

for_begin31.preheader:                            ; preds = %for_end33, %for_begin28.preheader
  %indvars.iv8 = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next9, %for_end33 ]
  %300 = mul nuw nsw i64 %indvars.iv8, 676
  br label %for_begin34.preheader

for_end30:                                        ; preds = %for_end33
  ret void

for_begin34.preheader:                            ; preds = %for_begin34.preheader, %for_begin31.preheader
  %indvars.iv = phi i64 [ 0, %for_begin31.preheader ], [ %indvars.iv.next, %for_begin34.preheader ]
  %301 = mul nuw nsw i64 %indvars.iv, 26
  %302 = add nuw nsw i64 %301, %300
  %303 = getelementptr inbounds float, float* %60, i64 %302
  %304 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %302
  %305 = getelementptr inbounds float, float* %238, i64 %302
  %306 = bitcast float* %303 to <4 x float>*
  %307 = load <4 x float>, <4 x float>* %306, align 4, !tbaa !112
  %308 = bitcast float* %304 to <4 x float>*
  %309 = load <4 x float>, <4 x float>* %308, align 8, !tbaa !109
  %310 = fdiv <4 x float> %307, %309
  %311 = bitcast float* %305 to <4 x float>*
  store <4 x float> %310, <4 x float>* %311, align 4, !tbaa !115
  %312 = add nuw nsw i64 %302, 4
  %313 = getelementptr inbounds float, float* %60, i64 %312
  %314 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %312
  %315 = getelementptr inbounds float, float* %238, i64 %312
  %316 = bitcast float* %313 to <4 x float>*
  %317 = load <4 x float>, <4 x float>* %316, align 4, !tbaa !112
  %318 = bitcast float* %314 to <4 x float>*
  %319 = load <4 x float>, <4 x float>* %318, align 8, !tbaa !109
  %320 = fdiv <4 x float> %317, %319
  %321 = bitcast float* %315 to <4 x float>*
  store <4 x float> %320, <4 x float>* %321, align 4, !tbaa !115
  %322 = add nuw nsw i64 %302, 8
  %323 = getelementptr inbounds float, float* %60, i64 %322
  %324 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %322
  %325 = getelementptr inbounds float, float* %238, i64 %322
  %326 = bitcast float* %323 to <4 x float>*
  %327 = load <4 x float>, <4 x float>* %326, align 4, !tbaa !112
  %328 = bitcast float* %324 to <4 x float>*
  %329 = load <4 x float>, <4 x float>* %328, align 8, !tbaa !109
  %330 = fdiv <4 x float> %327, %329
  %331 = bitcast float* %325 to <4 x float>*
  store <4 x float> %330, <4 x float>* %331, align 4, !tbaa !115
  %332 = add nuw nsw i64 %302, 12
  %333 = getelementptr inbounds float, float* %60, i64 %332
  %334 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %332
  %335 = getelementptr inbounds float, float* %238, i64 %332
  %336 = bitcast float* %333 to <4 x float>*
  %337 = load <4 x float>, <4 x float>* %336, align 4, !tbaa !112
  %338 = bitcast float* %334 to <4 x float>*
  %339 = load <4 x float>, <4 x float>* %338, align 8, !tbaa !109
  %340 = fdiv <4 x float> %337, %339
  %341 = bitcast float* %335 to <4 x float>*
  store <4 x float> %340, <4 x float>* %341, align 4, !tbaa !115
  %342 = add nuw nsw i64 %302, 16
  %343 = getelementptr inbounds float, float* %60, i64 %342
  %344 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %342
  %345 = getelementptr inbounds float, float* %238, i64 %342
  %346 = bitcast float* %343 to <4 x float>*
  %347 = load <4 x float>, <4 x float>* %346, align 4, !tbaa !112
  %348 = bitcast float* %344 to <4 x float>*
  %349 = load <4 x float>, <4 x float>* %348, align 8, !tbaa !109
  %350 = fdiv <4 x float> %347, %349
  %351 = bitcast float* %345 to <4 x float>*
  store <4 x float> %350, <4 x float>* %351, align 4, !tbaa !115
  %352 = add nuw nsw i64 %302, 20
  %353 = getelementptr inbounds float, float* %60, i64 %352
  %354 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %352
  %355 = getelementptr inbounds float, float* %238, i64 %352
  %356 = bitcast float* %353 to <4 x float>*
  %357 = load <4 x float>, <4 x float>* %356, align 4, !tbaa !112
  %358 = bitcast float* %354 to <4 x float>*
  %359 = load <4 x float>, <4 x float>* %358, align 8, !tbaa !109
  %360 = fdiv <4 x float> %357, %359
  %361 = bitcast float* %355 to <4 x float>*
  store <4 x float> %360, <4 x float>* %361, align 4, !tbaa !115
  %362 = add nuw nsw i64 %302, 24
  %363 = getelementptr inbounds float, float* %60, i64 %362
  %364 = load float, float* %363, align 4, !tbaa !112
  %365 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %362
  %366 = load float, float* %365, align 8, !tbaa !109
  %367 = fdiv float %364, %366
  %368 = getelementptr inbounds float, float* %238, i64 %362
  store float %367, float* %368, align 4, !tbaa !115
  %369 = add nuw nsw i64 %302, 25
  %370 = getelementptr inbounds float, float* %60, i64 %369
  %371 = load float, float* %370, align 4, !tbaa !112
  %372 = getelementptr inbounds [173056 x float], [173056 x float]* %2, i64 0, i64 %369
  %373 = load float, float* %372, align 4, !tbaa !109
  %374 = fdiv float %371, %373
  %375 = getelementptr inbounds float, float* %238, i64 %369
  store float %374, float* %375, align 4, !tbaa !115
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 26
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
  %6 = mul nuw nsw i64 %indvars.iv4, 2916
  %7 = getelementptr inbounds float, float* %3, i64 %indvars.iv4
  %8 = load float, float* %7, align 4, !tbaa !118
  %broadcast.splatinsert7 = insertelement <4 x float> undef, float %8, i32 0
  %broadcast.splat8 = shufflevector <4 x float> %broadcast.splatinsert7, <4 x float> undef, <4 x i32> zeroinitializer
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv1 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next2, %for_begin4.preheader ]
  %9 = mul nuw nsw i64 %indvars.iv1, 54
  %10 = add nuw nsw i64 %9, %6
  %11 = getelementptr inbounds float, float* %4, i64 %10
  %12 = bitcast float* %11 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %12, align 4, !tbaa !121
  %13 = fadd <4 x float> %broadcast.splat8, %wide.load
  %14 = getelementptr inbounds float, float* %5, i64 %10
  %15 = bitcast float* %14 to <4 x float>*
  store <4 x float> %13, <4 x float>* %15, align 4, !tbaa !124
  %16 = add nuw nsw i64 %10, 4
  %17 = getelementptr inbounds float, float* %4, i64 %16
  %18 = bitcast float* %17 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %18, align 4, !tbaa !121
  %19 = fadd <4 x float> %broadcast.splat8, %wide.load.1
  %20 = getelementptr inbounds float, float* %5, i64 %16
  %21 = bitcast float* %20 to <4 x float>*
  store <4 x float> %19, <4 x float>* %21, align 4, !tbaa !124
  %22 = add nuw nsw i64 %10, 8
  %23 = getelementptr inbounds float, float* %4, i64 %22
  %24 = bitcast float* %23 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %24, align 4, !tbaa !121
  %25 = fadd <4 x float> %broadcast.splat8, %wide.load.2
  %26 = getelementptr inbounds float, float* %5, i64 %22
  %27 = bitcast float* %26 to <4 x float>*
  store <4 x float> %25, <4 x float>* %27, align 4, !tbaa !124
  %28 = add nuw nsw i64 %10, 12
  %29 = getelementptr inbounds float, float* %4, i64 %28
  %30 = bitcast float* %29 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %30, align 4, !tbaa !121
  %31 = fadd <4 x float> %broadcast.splat8, %wide.load.3
  %32 = getelementptr inbounds float, float* %5, i64 %28
  %33 = bitcast float* %32 to <4 x float>*
  store <4 x float> %31, <4 x float>* %33, align 4, !tbaa !124
  %34 = add nuw nsw i64 %10, 16
  %35 = getelementptr inbounds float, float* %4, i64 %34
  %36 = bitcast float* %35 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %36, align 4, !tbaa !121
  %37 = fadd <4 x float> %broadcast.splat8, %wide.load.4
  %38 = getelementptr inbounds float, float* %5, i64 %34
  %39 = bitcast float* %38 to <4 x float>*
  store <4 x float> %37, <4 x float>* %39, align 4, !tbaa !124
  %40 = add nuw nsw i64 %10, 20
  %41 = getelementptr inbounds float, float* %4, i64 %40
  %42 = bitcast float* %41 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %42, align 4, !tbaa !121
  %43 = fadd <4 x float> %broadcast.splat8, %wide.load.5
  %44 = getelementptr inbounds float, float* %5, i64 %40
  %45 = bitcast float* %44 to <4 x float>*
  store <4 x float> %43, <4 x float>* %45, align 4, !tbaa !124
  %46 = add nuw nsw i64 %10, 24
  %47 = getelementptr inbounds float, float* %4, i64 %46
  %48 = bitcast float* %47 to <4 x float>*
  %wide.load.6 = load <4 x float>, <4 x float>* %48, align 4, !tbaa !121
  %49 = fadd <4 x float> %broadcast.splat8, %wide.load.6
  %50 = getelementptr inbounds float, float* %5, i64 %46
  %51 = bitcast float* %50 to <4 x float>*
  store <4 x float> %49, <4 x float>* %51, align 4, !tbaa !124
  %52 = add nuw nsw i64 %10, 28
  %53 = getelementptr inbounds float, float* %4, i64 %52
  %54 = bitcast float* %53 to <4 x float>*
  %wide.load.7 = load <4 x float>, <4 x float>* %54, align 4, !tbaa !121
  %55 = fadd <4 x float> %broadcast.splat8, %wide.load.7
  %56 = getelementptr inbounds float, float* %5, i64 %52
  %57 = bitcast float* %56 to <4 x float>*
  store <4 x float> %55, <4 x float>* %57, align 4, !tbaa !124
  %58 = add nuw nsw i64 %10, 32
  %59 = getelementptr inbounds float, float* %4, i64 %58
  %60 = bitcast float* %59 to <4 x float>*
  %wide.load.8 = load <4 x float>, <4 x float>* %60, align 4, !tbaa !121
  %61 = fadd <4 x float> %broadcast.splat8, %wide.load.8
  %62 = getelementptr inbounds float, float* %5, i64 %58
  %63 = bitcast float* %62 to <4 x float>*
  store <4 x float> %61, <4 x float>* %63, align 4, !tbaa !124
  %64 = add nuw nsw i64 %10, 36
  %65 = getelementptr inbounds float, float* %4, i64 %64
  %66 = bitcast float* %65 to <4 x float>*
  %wide.load.9 = load <4 x float>, <4 x float>* %66, align 4, !tbaa !121
  %67 = fadd <4 x float> %broadcast.splat8, %wide.load.9
  %68 = getelementptr inbounds float, float* %5, i64 %64
  %69 = bitcast float* %68 to <4 x float>*
  store <4 x float> %67, <4 x float>* %69, align 4, !tbaa !124
  %70 = add nuw nsw i64 %10, 40
  %71 = getelementptr inbounds float, float* %4, i64 %70
  %72 = bitcast float* %71 to <4 x float>*
  %wide.load.10 = load <4 x float>, <4 x float>* %72, align 4, !tbaa !121
  %73 = fadd <4 x float> %broadcast.splat8, %wide.load.10
  %74 = getelementptr inbounds float, float* %5, i64 %70
  %75 = bitcast float* %74 to <4 x float>*
  store <4 x float> %73, <4 x float>* %75, align 4, !tbaa !124
  %76 = add nuw nsw i64 %10, 44
  %77 = getelementptr inbounds float, float* %4, i64 %76
  %78 = bitcast float* %77 to <4 x float>*
  %wide.load.11 = load <4 x float>, <4 x float>* %78, align 4, !tbaa !121
  %79 = fadd <4 x float> %broadcast.splat8, %wide.load.11
  %80 = getelementptr inbounds float, float* %5, i64 %76
  %81 = bitcast float* %80 to <4 x float>*
  store <4 x float> %79, <4 x float>* %81, align 4, !tbaa !124
  %82 = add nuw nsw i64 %10, 48
  %83 = getelementptr inbounds float, float* %4, i64 %82
  %84 = bitcast float* %83 to <4 x float>*
  %wide.load.12 = load <4 x float>, <4 x float>* %84, align 4, !tbaa !121
  %85 = fadd <4 x float> %broadcast.splat8, %wide.load.12
  %86 = getelementptr inbounds float, float* %5, i64 %82
  %87 = bitcast float* %86 to <4 x float>*
  store <4 x float> %85, <4 x float>* %87, align 4, !tbaa !124
  %88 = add nuw nsw i64 %10, 52
  %89 = getelementptr inbounds float, float* %4, i64 %88
  %90 = load float, float* %89, align 4, !tbaa !121
  %91 = fadd float %8, %90
  %92 = getelementptr inbounds float, float* %5, i64 %88
  store float %91, float* %92, align 4, !tbaa !124
  %93 = add nuw nsw i64 %10, 53
  %94 = getelementptr inbounds float, float* %4, i64 %93
  %95 = load float, float* %94, align 4, !tbaa !121
  %96 = fadd float %8, %95
  %97 = getelementptr inbounds float, float* %5, i64 %93
  store float %96, float* %97, align 4, !tbaa !124
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 54
  br i1 %exitcond3, label %for_end3, label %for_begin4.preheader, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 96
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !28
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
  %3 = alloca [12 x <8 x float>], align 16
  %4 = alloca [110592 x <8 x float>], align 16
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
  store float 0.000000e+00, float* %17, align 16, !tbaa !127
  %18 = or i64 %7, 1
  %19 = add nsw i64 %16, 1
  %20 = getelementptr inbounds float, float* %6, i64 %19
  %21 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %18
  %22 = bitcast float* %20 to <4 x i32>*
  %23 = load <4 x i32>, <4 x i32>* %22, align 4, !tbaa !130
  %24 = bitcast float* %21 to <4 x i32>*
  store <4 x i32> %23, <4 x i32>* %24, align 4, !tbaa !127
  %25 = or i64 %7, 5
  %26 = add nsw i64 %16, 5
  %27 = getelementptr inbounds float, float* %6, i64 %26
  %28 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %25
  %29 = bitcast float* %27 to <4 x i32>*
  %30 = load <4 x i32>, <4 x i32>* %29, align 4, !tbaa !130
  %31 = bitcast float* %28 to <4 x i32>*
  store <4 x i32> %30, <4 x i32>* %31, align 4, !tbaa !127
  %32 = or i64 %7, 9
  %33 = add nsw i64 %16, 9
  %34 = getelementptr inbounds float, float* %6, i64 %33
  %35 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %32
  %36 = bitcast float* %34 to <4 x i32>*
  %37 = load <4 x i32>, <4 x i32>* %36, align 4, !tbaa !130
  %38 = bitcast float* %35 to <4 x i32>*
  store <4 x i32> %37, <4 x i32>* %38, align 4, !tbaa !127
  %39 = or i64 %7, 13
  %40 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %39
  store float 0.000000e+00, float* %40, align 4, !tbaa !127
  %41 = or i64 %7, 14
  %42 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %41
  store float 0.000000e+00, float* %42, align 8, !tbaa !127
  %43 = or i64 %7, 15
  %44 = add nsw i64 %16, 145
  %45 = getelementptr inbounds float, float* %6, i64 %44
  %46 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %43
  %47 = bitcast float* %45 to <4 x i32>*
  %48 = load <4 x i32>, <4 x i32>* %47, align 4, !tbaa !130
  %49 = bitcast float* %46 to <4 x i32>*
  store <4 x i32> %48, <4 x i32>* %49, align 4, !tbaa !127
  %50 = add nuw nsw i64 %41, 5
  %51 = add nsw i64 %16, 149
  %52 = getelementptr inbounds float, float* %6, i64 %51
  %53 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %50
  %54 = bitcast float* %52 to <4 x i32>*
  %55 = load <4 x i32>, <4 x i32>* %54, align 4, !tbaa !130
  %56 = bitcast float* %53 to <4 x i32>*
  store <4 x i32> %55, <4 x i32>* %56, align 4, !tbaa !127
  %57 = add nuw nsw i64 %41, 9
  %58 = add nsw i64 %16, 153
  %59 = getelementptr inbounds float, float* %6, i64 %58
  %60 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %57
  %61 = bitcast float* %59 to <4 x i32>*
  %62 = load <4 x i32>, <4 x i32>* %61, align 4, !tbaa !130
  %63 = bitcast float* %60 to <4 x i32>*
  store <4 x i32> %62, <4 x i32>* %63, align 4, !tbaa !127
  %64 = add nuw nsw i64 %41, 13
  %65 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %64
  store float 0.000000e+00, float* %65, align 4, !tbaa !127
  %66 = add nuw nsw i64 %7, 28
  %67 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %66
  store float 0.000000e+00, float* %67, align 16, !tbaa !127
  %68 = or i64 %66, 1
  %69 = add nsw i64 %16, 289
  %70 = getelementptr inbounds float, float* %6, i64 %69
  %71 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %68
  %72 = bitcast float* %70 to <4 x i32>*
  %73 = load <4 x i32>, <4 x i32>* %72, align 4, !tbaa !130
  %74 = bitcast float* %71 to <4 x i32>*
  store <4 x i32> %73, <4 x i32>* %74, align 4, !tbaa !127
  %75 = add nuw nsw i64 %7, 33
  %76 = add nsw i64 %16, 293
  %77 = getelementptr inbounds float, float* %6, i64 %76
  %78 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %75
  %79 = bitcast float* %77 to <4 x i32>*
  %80 = load <4 x i32>, <4 x i32>* %79, align 4, !tbaa !130
  %81 = bitcast float* %78 to <4 x i32>*
  store <4 x i32> %80, <4 x i32>* %81, align 4, !tbaa !127
  %82 = add nuw nsw i64 %7, 37
  %83 = add nsw i64 %16, 297
  %84 = getelementptr inbounds float, float* %6, i64 %83
  %85 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %82
  %86 = bitcast float* %84 to <4 x i32>*
  %87 = load <4 x i32>, <4 x i32>* %86, align 4, !tbaa !130
  %88 = bitcast float* %85 to <4 x i32>*
  store <4 x i32> %87, <4 x i32>* %88, align 4, !tbaa !127
  %89 = add nuw nsw i64 %7, 41
  %90 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %89
  store float 0.000000e+00, float* %90, align 4, !tbaa !127
  %91 = add nuw nsw i64 %7, 42
  %92 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %91
  store float 0.000000e+00, float* %92, align 8, !tbaa !127
  %93 = or i64 %91, 1
  %94 = add nsw i64 %16, 433
  %95 = getelementptr inbounds float, float* %6, i64 %94
  %96 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %93
  %97 = bitcast float* %95 to <4 x i32>*
  %98 = load <4 x i32>, <4 x i32>* %97, align 4, !tbaa !130
  %99 = bitcast float* %96 to <4 x i32>*
  store <4 x i32> %98, <4 x i32>* %99, align 4, !tbaa !127
  %100 = add nuw nsw i64 %7, 47
  %101 = add nsw i64 %16, 437
  %102 = getelementptr inbounds float, float* %6, i64 %101
  %103 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %100
  %104 = bitcast float* %102 to <4 x i32>*
  %105 = load <4 x i32>, <4 x i32>* %104, align 4, !tbaa !130
  %106 = bitcast float* %103 to <4 x i32>*
  store <4 x i32> %105, <4 x i32>* %106, align 4, !tbaa !127
  %107 = add nuw nsw i64 %7, 51
  %108 = add nsw i64 %16, 441
  %109 = getelementptr inbounds float, float* %6, i64 %108
  %110 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %107
  %111 = bitcast float* %109 to <4 x i32>*
  %112 = load <4 x i32>, <4 x i32>* %111, align 4, !tbaa !130
  %113 = bitcast float* %110 to <4 x i32>*
  store <4 x i32> %112, <4 x i32>* %113, align 4, !tbaa !127
  %114 = add nuw nsw i64 %7, 55
  %115 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %114
  store float 0.000000e+00, float* %115, align 4, !tbaa !127
  %116 = add nuw nsw i64 %7, 56
  %117 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %116
  store float 0.000000e+00, float* %117, align 16, !tbaa !127
  %118 = or i64 %116, 1
  %119 = add nsw i64 %16, 577
  %120 = getelementptr inbounds float, float* %6, i64 %119
  %121 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %118
  %122 = bitcast float* %120 to <4 x i32>*
  %123 = load <4 x i32>, <4 x i32>* %122, align 4, !tbaa !130
  %124 = bitcast float* %121 to <4 x i32>*
  store <4 x i32> %123, <4 x i32>* %124, align 4, !tbaa !127
  %125 = add nuw nsw i64 %7, 61
  %126 = add nsw i64 %16, 581
  %127 = getelementptr inbounds float, float* %6, i64 %126
  %128 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %125
  %129 = bitcast float* %127 to <4 x i32>*
  %130 = load <4 x i32>, <4 x i32>* %129, align 4, !tbaa !130
  %131 = bitcast float* %128 to <4 x i32>*
  store <4 x i32> %130, <4 x i32>* %131, align 4, !tbaa !127
  %132 = add nuw nsw i64 %7, 65
  %133 = add nsw i64 %16, 585
  %134 = getelementptr inbounds float, float* %6, i64 %133
  %135 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %132
  %136 = bitcast float* %134 to <4 x i32>*
  %137 = load <4 x i32>, <4 x i32>* %136, align 4, !tbaa !130
  %138 = bitcast float* %135 to <4 x i32>*
  store <4 x i32> %137, <4 x i32>* %138, align 4, !tbaa !127
  %139 = add nuw nsw i64 %7, 69
  %140 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %139
  store float 0.000000e+00, float* %140, align 4, !tbaa !127
  %141 = add nuw nsw i64 %7, 70
  %142 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %141
  store float 0.000000e+00, float* %142, align 8, !tbaa !127
  %143 = or i64 %141, 1
  %144 = add nsw i64 %16, 721
  %145 = getelementptr inbounds float, float* %6, i64 %144
  %146 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %143
  %147 = bitcast float* %145 to <4 x i32>*
  %148 = load <4 x i32>, <4 x i32>* %147, align 4, !tbaa !130
  %149 = bitcast float* %146 to <4 x i32>*
  store <4 x i32> %148, <4 x i32>* %149, align 4, !tbaa !127
  %150 = add nuw nsw i64 %7, 75
  %151 = add nsw i64 %16, 725
  %152 = getelementptr inbounds float, float* %6, i64 %151
  %153 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %150
  %154 = bitcast float* %152 to <4 x i32>*
  %155 = load <4 x i32>, <4 x i32>* %154, align 4, !tbaa !130
  %156 = bitcast float* %153 to <4 x i32>*
  store <4 x i32> %155, <4 x i32>* %156, align 4, !tbaa !127
  %157 = add nuw nsw i64 %7, 79
  %158 = add nsw i64 %16, 729
  %159 = getelementptr inbounds float, float* %6, i64 %158
  %160 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %157
  %161 = bitcast float* %159 to <4 x i32>*
  %162 = load <4 x i32>, <4 x i32>* %161, align 4, !tbaa !130
  %163 = bitcast float* %160 to <4 x i32>*
  store <4 x i32> %162, <4 x i32>* %163, align 4, !tbaa !127
  %164 = add nuw nsw i64 %7, 83
  %165 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %164
  store float 0.000000e+00, float* %165, align 4, !tbaa !127
  %166 = add nuw nsw i64 %7, 84
  %167 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %166
  store float 0.000000e+00, float* %167, align 16, !tbaa !127
  %168 = or i64 %166, 1
  %169 = add nsw i64 %16, 865
  %170 = getelementptr inbounds float, float* %6, i64 %169
  %171 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %168
  %172 = bitcast float* %170 to <4 x i32>*
  %173 = load <4 x i32>, <4 x i32>* %172, align 4, !tbaa !130
  %174 = bitcast float* %171 to <4 x i32>*
  store <4 x i32> %173, <4 x i32>* %174, align 4, !tbaa !127
  %175 = add nuw nsw i64 %7, 89
  %176 = add nsw i64 %16, 869
  %177 = getelementptr inbounds float, float* %6, i64 %176
  %178 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %175
  %179 = bitcast float* %177 to <4 x i32>*
  %180 = load <4 x i32>, <4 x i32>* %179, align 4, !tbaa !130
  %181 = bitcast float* %178 to <4 x i32>*
  store <4 x i32> %180, <4 x i32>* %181, align 4, !tbaa !127
  %182 = add nuw nsw i64 %7, 93
  %183 = add nsw i64 %16, 873
  %184 = getelementptr inbounds float, float* %6, i64 %183
  %185 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %182
  %186 = bitcast float* %184 to <4 x i32>*
  %187 = load <4 x i32>, <4 x i32>* %186, align 4, !tbaa !130
  %188 = bitcast float* %185 to <4 x i32>*
  store <4 x i32> %187, <4 x i32>* %188, align 4, !tbaa !127
  %189 = add nuw nsw i64 %7, 97
  %190 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %189
  store float 0.000000e+00, float* %190, align 4, !tbaa !127
  %191 = add nuw nsw i64 %7, 98
  %192 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %191
  store float 0.000000e+00, float* %192, align 8, !tbaa !127
  %193 = or i64 %191, 1
  %194 = add nsw i64 %16, 1009
  %195 = getelementptr inbounds float, float* %6, i64 %194
  %196 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %193
  %197 = bitcast float* %195 to <4 x i32>*
  %198 = load <4 x i32>, <4 x i32>* %197, align 4, !tbaa !130
  %199 = bitcast float* %196 to <4 x i32>*
  store <4 x i32> %198, <4 x i32>* %199, align 4, !tbaa !127
  %200 = add nuw nsw i64 %7, 103
  %201 = add nsw i64 %16, 1013
  %202 = getelementptr inbounds float, float* %6, i64 %201
  %203 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %200
  %204 = bitcast float* %202 to <4 x i32>*
  %205 = load <4 x i32>, <4 x i32>* %204, align 4, !tbaa !130
  %206 = bitcast float* %203 to <4 x i32>*
  store <4 x i32> %205, <4 x i32>* %206, align 4, !tbaa !127
  %207 = add nuw nsw i64 %7, 107
  %208 = add nsw i64 %16, 1017
  %209 = getelementptr inbounds float, float* %6, i64 %208
  %210 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %207
  %211 = bitcast float* %209 to <4 x i32>*
  %212 = load <4 x i32>, <4 x i32>* %211, align 4, !tbaa !130
  %213 = bitcast float* %210 to <4 x i32>*
  store <4 x i32> %212, <4 x i32>* %213, align 4, !tbaa !127
  %214 = add nuw nsw i64 %7, 111
  %215 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %214
  store float 0.000000e+00, float* %215, align 4, !tbaa !127
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
  %exitcond134 = icmp eq i32 %255, 144
  br i1 %exitcond134, label %for_begin19.preheader, label %for_begin10.preheader, !prof !28

for_begin16.preheader:                            ; preds = %for_begin16.preheader, %for_begin13.preheader
  %indvars.iv128 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next129, %for_begin16.preheader ]
  %256 = shl i64 %indvars.iv128, 6
  %257 = add nuw nsw i64 %252, %256
  %258 = add nuw nsw i64 %254, %indvars.iv128
  %259 = getelementptr inbounds float, float* %216, i64 %258
  %260 = load float, float* %259, align 4, !tbaa !133
  %261 = insertelement <8 x float> undef, float %260, i32 0
  %262 = shl i64 %258, 32
  %sext = add i64 %262, 9895604649984
  %263 = ashr exact i64 %sext, 32
  %264 = getelementptr inbounds float, float* %216, i64 %263
  %265 = load float, float* %264, align 4, !tbaa !133
  %266 = insertelement <8 x float> %261, float %265, i32 1
  %267 = shl i64 %258, 32
  %sext151 = add i64 %267, 19791209299968
  %268 = ashr exact i64 %sext151, 32
  %269 = getelementptr inbounds float, float* %216, i64 %268
  %270 = load float, float* %269, align 4, !tbaa !133
  %271 = insertelement <8 x float> %266, float %270, i32 2
  %272 = shl i64 %258, 32
  %sext152 = add i64 %272, 29686813949952
  %273 = ashr exact i64 %sext152, 32
  %274 = getelementptr inbounds float, float* %216, i64 %273
  %275 = load float, float* %274, align 4, !tbaa !133
  %276 = insertelement <8 x float> %271, float %275, i32 3
  %277 = shl i64 %258, 32
  %sext153 = add i64 %277, 39582418599936
  %278 = ashr exact i64 %sext153, 32
  %279 = getelementptr inbounds float, float* %216, i64 %278
  %280 = load float, float* %279, align 4, !tbaa !133
  %281 = insertelement <8 x float> %276, float %280, i32 4
  %282 = shl i64 %258, 32
  %sext154 = add i64 %282, 49478023249920
  %283 = ashr exact i64 %sext154, 32
  %284 = getelementptr inbounds float, float* %216, i64 %283
  %285 = load float, float* %284, align 4, !tbaa !133
  %286 = insertelement <8 x float> %281, float %285, i32 5
  %287 = shl i64 %258, 32
  %sext155 = add i64 %287, 59373627899904
  %288 = ashr exact i64 %sext155, 32
  %289 = getelementptr inbounds float, float* %216, i64 %288
  %290 = load float, float* %289, align 4, !tbaa !133
  %291 = insertelement <8 x float> %286, float %290, i32 6
  %292 = shl i64 %258, 32
  %sext156 = add i64 %292, 69269232549888
  %293 = ashr exact i64 %sext156, 32
  %294 = getelementptr inbounds float, float* %216, i64 %293
  %295 = load float, float* %294, align 4, !tbaa !133
  %296 = insertelement <8 x float> %291, float %295, i32 7
  %297 = getelementptr inbounds [110592 x <8 x float>], [110592 x <8 x float>]* %4, i64 0, i64 0, i64 %257
  %298 = bitcast float* %297 to <8 x float>*
  store <8 x float> %296, <8 x float>* %298, align 16, !tbaa !136
  %299 = or i64 %257, 8
  %300 = add nuw nsw i64 %258, 9
  %301 = getelementptr inbounds float, float* %216, i64 %300
  %302 = load float, float* %301, align 4, !tbaa !133
  %303 = insertelement <8 x float> undef, float %302, i32 0
  %304 = shl i64 %258, 32
  %sext157 = add i64 %304, 9934259355648
  %305 = ashr exact i64 %sext157, 32
  %306 = getelementptr inbounds float, float* %216, i64 %305
  %307 = load float, float* %306, align 4, !tbaa !133
  %308 = insertelement <8 x float> %303, float %307, i32 1
  %309 = shl i64 %258, 32
  %sext158 = add i64 %309, 19829864005632
  %310 = ashr exact i64 %sext158, 32
  %311 = getelementptr inbounds float, float* %216, i64 %310
  %312 = load float, float* %311, align 4, !tbaa !133
  %313 = insertelement <8 x float> %308, float %312, i32 2
  %314 = shl i64 %258, 32
  %sext159 = add i64 %314, 29725468655616
  %315 = ashr exact i64 %sext159, 32
  %316 = getelementptr inbounds float, float* %216, i64 %315
  %317 = load float, float* %316, align 4, !tbaa !133
  %318 = insertelement <8 x float> %313, float %317, i32 3
  %319 = shl i64 %258, 32
  %sext160 = add i64 %319, 39621073305600
  %320 = ashr exact i64 %sext160, 32
  %321 = getelementptr inbounds float, float* %216, i64 %320
  %322 = load float, float* %321, align 4, !tbaa !133
  %323 = insertelement <8 x float> %318, float %322, i32 4
  %324 = shl i64 %258, 32
  %sext161 = add i64 %324, 49516677955584
  %325 = ashr exact i64 %sext161, 32
  %326 = getelementptr inbounds float, float* %216, i64 %325
  %327 = load float, float* %326, align 4, !tbaa !133
  %328 = insertelement <8 x float> %323, float %327, i32 5
  %329 = shl i64 %258, 32
  %sext162 = add i64 %329, 59412282605568
  %330 = ashr exact i64 %sext162, 32
  %331 = getelementptr inbounds float, float* %216, i64 %330
  %332 = load float, float* %331, align 4, !tbaa !133
  %333 = insertelement <8 x float> %328, float %332, i32 6
  %334 = shl i64 %258, 32
  %sext163 = add i64 %334, 69307887255552
  %335 = ashr exact i64 %sext163, 32
  %336 = getelementptr inbounds float, float* %216, i64 %335
  %337 = load float, float* %336, align 4, !tbaa !133
  %338 = insertelement <8 x float> %333, float %337, i32 7
  %339 = getelementptr inbounds [110592 x <8 x float>], [110592 x <8 x float>]* %4, i64 0, i64 0, i64 %299
  %340 = bitcast float* %339 to <8 x float>*
  store <8 x float> %338, <8 x float>* %340, align 16, !tbaa !136
  %341 = or i64 %257, 16
  %342 = add nuw nsw i64 %258, 18
  %343 = getelementptr inbounds float, float* %216, i64 %342
  %344 = load float, float* %343, align 4, !tbaa !133
  %345 = insertelement <8 x float> undef, float %344, i32 0
  %346 = shl i64 %258, 32
  %sext164 = add i64 %346, 9972914061312
  %347 = ashr exact i64 %sext164, 32
  %348 = getelementptr inbounds float, float* %216, i64 %347
  %349 = load float, float* %348, align 4, !tbaa !133
  %350 = insertelement <8 x float> %345, float %349, i32 1
  %351 = shl i64 %258, 32
  %sext165 = add i64 %351, 19868518711296
  %352 = ashr exact i64 %sext165, 32
  %353 = getelementptr inbounds float, float* %216, i64 %352
  %354 = load float, float* %353, align 4, !tbaa !133
  %355 = insertelement <8 x float> %350, float %354, i32 2
  %356 = shl i64 %258, 32
  %sext166 = add i64 %356, 29764123361280
  %357 = ashr exact i64 %sext166, 32
  %358 = getelementptr inbounds float, float* %216, i64 %357
  %359 = load float, float* %358, align 4, !tbaa !133
  %360 = insertelement <8 x float> %355, float %359, i32 3
  %361 = shl i64 %258, 32
  %sext167 = add i64 %361, 39659728011264
  %362 = ashr exact i64 %sext167, 32
  %363 = getelementptr inbounds float, float* %216, i64 %362
  %364 = load float, float* %363, align 4, !tbaa !133
  %365 = insertelement <8 x float> %360, float %364, i32 4
  %366 = shl i64 %258, 32
  %sext168 = add i64 %366, 49555332661248
  %367 = ashr exact i64 %sext168, 32
  %368 = getelementptr inbounds float, float* %216, i64 %367
  %369 = load float, float* %368, align 4, !tbaa !133
  %370 = insertelement <8 x float> %365, float %369, i32 5
  %371 = shl i64 %258, 32
  %sext169 = add i64 %371, 59450937311232
  %372 = ashr exact i64 %sext169, 32
  %373 = getelementptr inbounds float, float* %216, i64 %372
  %374 = load float, float* %373, align 4, !tbaa !133
  %375 = insertelement <8 x float> %370, float %374, i32 6
  %376 = shl i64 %258, 32
  %sext170 = add i64 %376, 69346541961216
  %377 = ashr exact i64 %sext170, 32
  %378 = getelementptr inbounds float, float* %216, i64 %377
  %379 = load float, float* %378, align 4, !tbaa !133
  %380 = insertelement <8 x float> %375, float %379, i32 7
  %381 = getelementptr inbounds [110592 x <8 x float>], [110592 x <8 x float>]* %4, i64 0, i64 0, i64 %341
  %382 = bitcast float* %381 to <8 x float>*
  store <8 x float> %380, <8 x float>* %382, align 16, !tbaa !136
  %383 = or i64 %257, 24
  %384 = add nuw nsw i64 %258, 27
  %385 = getelementptr inbounds float, float* %216, i64 %384
  %386 = load float, float* %385, align 4, !tbaa !133
  %387 = insertelement <8 x float> undef, float %386, i32 0
  %388 = shl i64 %258, 32
  %sext171 = add i64 %388, 10011568766976
  %389 = ashr exact i64 %sext171, 32
  %390 = getelementptr inbounds float, float* %216, i64 %389
  %391 = load float, float* %390, align 4, !tbaa !133
  %392 = insertelement <8 x float> %387, float %391, i32 1
  %393 = shl i64 %258, 32
  %sext172 = add i64 %393, 19907173416960
  %394 = ashr exact i64 %sext172, 32
  %395 = getelementptr inbounds float, float* %216, i64 %394
  %396 = load float, float* %395, align 4, !tbaa !133
  %397 = insertelement <8 x float> %392, float %396, i32 2
  %398 = shl i64 %258, 32
  %sext173 = add i64 %398, 29802778066944
  %399 = ashr exact i64 %sext173, 32
  %400 = getelementptr inbounds float, float* %216, i64 %399
  %401 = load float, float* %400, align 4, !tbaa !133
  %402 = insertelement <8 x float> %397, float %401, i32 3
  %403 = shl i64 %258, 32
  %sext174 = add i64 %403, 39698382716928
  %404 = ashr exact i64 %sext174, 32
  %405 = getelementptr inbounds float, float* %216, i64 %404
  %406 = load float, float* %405, align 4, !tbaa !133
  %407 = insertelement <8 x float> %402, float %406, i32 4
  %408 = shl i64 %258, 32
  %sext175 = add i64 %408, 49593987366912
  %409 = ashr exact i64 %sext175, 32
  %410 = getelementptr inbounds float, float* %216, i64 %409
  %411 = load float, float* %410, align 4, !tbaa !133
  %412 = insertelement <8 x float> %407, float %411, i32 5
  %413 = shl i64 %258, 32
  %sext176 = add i64 %413, 59489592016896
  %414 = ashr exact i64 %sext176, 32
  %415 = getelementptr inbounds float, float* %216, i64 %414
  %416 = load float, float* %415, align 4, !tbaa !133
  %417 = insertelement <8 x float> %412, float %416, i32 6
  %418 = shl i64 %258, 32
  %sext177 = add i64 %418, 69385196666880
  %419 = ashr exact i64 %sext177, 32
  %420 = getelementptr inbounds float, float* %216, i64 %419
  %421 = load float, float* %420, align 4, !tbaa !133
  %422 = insertelement <8 x float> %417, float %421, i32 7
  %423 = getelementptr inbounds [110592 x <8 x float>], [110592 x <8 x float>]* %4, i64 0, i64 0, i64 %383
  %424 = bitcast float* %423 to <8 x float>*
  store <8 x float> %422, <8 x float>* %424, align 16, !tbaa !136
  %425 = or i64 %257, 32
  %426 = add nuw nsw i64 %258, 36
  %427 = getelementptr inbounds float, float* %216, i64 %426
  %428 = load float, float* %427, align 4, !tbaa !133
  %429 = insertelement <8 x float> undef, float %428, i32 0
  %430 = shl i64 %258, 32
  %sext178 = add i64 %430, 10050223472640
  %431 = ashr exact i64 %sext178, 32
  %432 = getelementptr inbounds float, float* %216, i64 %431
  %433 = load float, float* %432, align 4, !tbaa !133
  %434 = insertelement <8 x float> %429, float %433, i32 1
  %435 = shl i64 %258, 32
  %sext179 = add i64 %435, 19945828122624
  %436 = ashr exact i64 %sext179, 32
  %437 = getelementptr inbounds float, float* %216, i64 %436
  %438 = load float, float* %437, align 4, !tbaa !133
  %439 = insertelement <8 x float> %434, float %438, i32 2
  %440 = shl i64 %258, 32
  %sext180 = add i64 %440, 29841432772608
  %441 = ashr exact i64 %sext180, 32
  %442 = getelementptr inbounds float, float* %216, i64 %441
  %443 = load float, float* %442, align 4, !tbaa !133
  %444 = insertelement <8 x float> %439, float %443, i32 3
  %445 = shl i64 %258, 32
  %sext181 = add i64 %445, 39737037422592
  %446 = ashr exact i64 %sext181, 32
  %447 = getelementptr inbounds float, float* %216, i64 %446
  %448 = load float, float* %447, align 4, !tbaa !133
  %449 = insertelement <8 x float> %444, float %448, i32 4
  %450 = shl i64 %258, 32
  %sext182 = add i64 %450, 49632642072576
  %451 = ashr exact i64 %sext182, 32
  %452 = getelementptr inbounds float, float* %216, i64 %451
  %453 = load float, float* %452, align 4, !tbaa !133
  %454 = insertelement <8 x float> %449, float %453, i32 5
  %455 = shl i64 %258, 32
  %sext183 = add i64 %455, 59528246722560
  %456 = ashr exact i64 %sext183, 32
  %457 = getelementptr inbounds float, float* %216, i64 %456
  %458 = load float, float* %457, align 4, !tbaa !133
  %459 = insertelement <8 x float> %454, float %458, i32 6
  %460 = shl i64 %258, 32
  %sext184 = add i64 %460, 69423851372544
  %461 = ashr exact i64 %sext184, 32
  %462 = getelementptr inbounds float, float* %216, i64 %461
  %463 = load float, float* %462, align 4, !tbaa !133
  %464 = insertelement <8 x float> %459, float %463, i32 7
  %465 = getelementptr inbounds [110592 x <8 x float>], [110592 x <8 x float>]* %4, i64 0, i64 0, i64 %425
  %466 = bitcast float* %465 to <8 x float>*
  store <8 x float> %464, <8 x float>* %466, align 16, !tbaa !136
  %467 = or i64 %257, 40
  %468 = add nuw nsw i64 %258, 45
  %469 = getelementptr inbounds float, float* %216, i64 %468
  %470 = load float, float* %469, align 4, !tbaa !133
  %471 = insertelement <8 x float> undef, float %470, i32 0
  %472 = shl i64 %258, 32
  %sext185 = add i64 %472, 10088878178304
  %473 = ashr exact i64 %sext185, 32
  %474 = getelementptr inbounds float, float* %216, i64 %473
  %475 = load float, float* %474, align 4, !tbaa !133
  %476 = insertelement <8 x float> %471, float %475, i32 1
  %477 = shl i64 %258, 32
  %sext186 = add i64 %477, 19984482828288
  %478 = ashr exact i64 %sext186, 32
  %479 = getelementptr inbounds float, float* %216, i64 %478
  %480 = load float, float* %479, align 4, !tbaa !133
  %481 = insertelement <8 x float> %476, float %480, i32 2
  %482 = shl i64 %258, 32
  %sext187 = add i64 %482, 29880087478272
  %483 = ashr exact i64 %sext187, 32
  %484 = getelementptr inbounds float, float* %216, i64 %483
  %485 = load float, float* %484, align 4, !tbaa !133
  %486 = insertelement <8 x float> %481, float %485, i32 3
  %487 = shl i64 %258, 32
  %sext188 = add i64 %487, 39775692128256
  %488 = ashr exact i64 %sext188, 32
  %489 = getelementptr inbounds float, float* %216, i64 %488
  %490 = load float, float* %489, align 4, !tbaa !133
  %491 = insertelement <8 x float> %486, float %490, i32 4
  %492 = shl i64 %258, 32
  %sext189 = add i64 %492, 49671296778240
  %493 = ashr exact i64 %sext189, 32
  %494 = getelementptr inbounds float, float* %216, i64 %493
  %495 = load float, float* %494, align 4, !tbaa !133
  %496 = insertelement <8 x float> %491, float %495, i32 5
  %497 = shl i64 %258, 32
  %sext190 = add i64 %497, 59566901428224
  %498 = ashr exact i64 %sext190, 32
  %499 = getelementptr inbounds float, float* %216, i64 %498
  %500 = load float, float* %499, align 4, !tbaa !133
  %501 = insertelement <8 x float> %496, float %500, i32 6
  %502 = shl i64 %258, 32
  %sext191 = add i64 %502, 69462506078208
  %503 = ashr exact i64 %sext191, 32
  %504 = getelementptr inbounds float, float* %216, i64 %503
  %505 = load float, float* %504, align 4, !tbaa !133
  %506 = insertelement <8 x float> %501, float %505, i32 7
  %507 = getelementptr inbounds [110592 x <8 x float>], [110592 x <8 x float>]* %4, i64 0, i64 0, i64 %467
  %508 = bitcast float* %507 to <8 x float>*
  store <8 x float> %506, <8 x float>* %508, align 16, !tbaa !136
  %509 = or i64 %257, 48
  %510 = add nuw nsw i64 %258, 54
  %511 = getelementptr inbounds float, float* %216, i64 %510
  %512 = load float, float* %511, align 4, !tbaa !133
  %513 = insertelement <8 x float> undef, float %512, i32 0
  %514 = shl i64 %258, 32
  %sext192 = add i64 %514, 10127532883968
  %515 = ashr exact i64 %sext192, 32
  %516 = getelementptr inbounds float, float* %216, i64 %515
  %517 = load float, float* %516, align 4, !tbaa !133
  %518 = insertelement <8 x float> %513, float %517, i32 1
  %519 = shl i64 %258, 32
  %sext193 = add i64 %519, 20023137533952
  %520 = ashr exact i64 %sext193, 32
  %521 = getelementptr inbounds float, float* %216, i64 %520
  %522 = load float, float* %521, align 4, !tbaa !133
  %523 = insertelement <8 x float> %518, float %522, i32 2
  %524 = shl i64 %258, 32
  %sext194 = add i64 %524, 29918742183936
  %525 = ashr exact i64 %sext194, 32
  %526 = getelementptr inbounds float, float* %216, i64 %525
  %527 = load float, float* %526, align 4, !tbaa !133
  %528 = insertelement <8 x float> %523, float %527, i32 3
  %529 = shl i64 %258, 32
  %sext195 = add i64 %529, 39814346833920
  %530 = ashr exact i64 %sext195, 32
  %531 = getelementptr inbounds float, float* %216, i64 %530
  %532 = load float, float* %531, align 4, !tbaa !133
  %533 = insertelement <8 x float> %528, float %532, i32 4
  %534 = shl i64 %258, 32
  %sext196 = add i64 %534, 49709951483904
  %535 = ashr exact i64 %sext196, 32
  %536 = getelementptr inbounds float, float* %216, i64 %535
  %537 = load float, float* %536, align 4, !tbaa !133
  %538 = insertelement <8 x float> %533, float %537, i32 5
  %539 = shl i64 %258, 32
  %sext197 = add i64 %539, 59605556133888
  %540 = ashr exact i64 %sext197, 32
  %541 = getelementptr inbounds float, float* %216, i64 %540
  %542 = load float, float* %541, align 4, !tbaa !133
  %543 = insertelement <8 x float> %538, float %542, i32 6
  %544 = shl i64 %258, 32
  %sext198 = add i64 %544, 69501160783872
  %545 = ashr exact i64 %sext198, 32
  %546 = getelementptr inbounds float, float* %216, i64 %545
  %547 = load float, float* %546, align 4, !tbaa !133
  %548 = insertelement <8 x float> %543, float %547, i32 7
  %549 = getelementptr inbounds [110592 x <8 x float>], [110592 x <8 x float>]* %4, i64 0, i64 0, i64 %509
  %550 = bitcast float* %549 to <8 x float>*
  store <8 x float> %548, <8 x float>* %550, align 16, !tbaa !136
  %551 = or i64 %257, 56
  %552 = add nuw nsw i64 %258, 63
  %553 = getelementptr inbounds float, float* %216, i64 %552
  %554 = load float, float* %553, align 4, !tbaa !133
  %555 = insertelement <8 x float> undef, float %554, i32 0
  %556 = shl i64 %258, 32
  %sext199 = add i64 %556, 10166187589632
  %557 = ashr exact i64 %sext199, 32
  %558 = getelementptr inbounds float, float* %216, i64 %557
  %559 = load float, float* %558, align 4, !tbaa !133
  %560 = insertelement <8 x float> %555, float %559, i32 1
  %561 = shl i64 %258, 32
  %sext200 = add i64 %561, 20061792239616
  %562 = ashr exact i64 %sext200, 32
  %563 = getelementptr inbounds float, float* %216, i64 %562
  %564 = load float, float* %563, align 4, !tbaa !133
  %565 = insertelement <8 x float> %560, float %564, i32 2
  %566 = shl i64 %258, 32
  %sext201 = add i64 %566, 29957396889600
  %567 = ashr exact i64 %sext201, 32
  %568 = getelementptr inbounds float, float* %216, i64 %567
  %569 = load float, float* %568, align 4, !tbaa !133
  %570 = insertelement <8 x float> %565, float %569, i32 3
  %571 = shl i64 %258, 32
  %sext202 = add i64 %571, 39853001539584
  %572 = ashr exact i64 %sext202, 32
  %573 = getelementptr inbounds float, float* %216, i64 %572
  %574 = load float, float* %573, align 4, !tbaa !133
  %575 = insertelement <8 x float> %570, float %574, i32 4
  %576 = shl i64 %258, 32
  %sext203 = add i64 %576, 49748606189568
  %577 = ashr exact i64 %sext203, 32
  %578 = getelementptr inbounds float, float* %216, i64 %577
  %579 = load float, float* %578, align 4, !tbaa !133
  %580 = insertelement <8 x float> %575, float %579, i32 5
  %581 = shl i64 %258, 32
  %sext204 = add i64 %581, 59644210839552
  %582 = ashr exact i64 %sext204, 32
  %583 = getelementptr inbounds float, float* %216, i64 %582
  %584 = load float, float* %583, align 4, !tbaa !133
  %585 = insertelement <8 x float> %580, float %584, i32 6
  %586 = shl i64 %258, 32
  %sext205 = add i64 %586, 69539815489536
  %587 = ashr exact i64 %sext205, 32
  %588 = getelementptr inbounds float, float* %216, i64 %587
  %589 = load float, float* %588, align 4, !tbaa !133
  %590 = insertelement <8 x float> %585, float %589, i32 7
  %591 = getelementptr inbounds [110592 x <8 x float>], [110592 x <8 x float>]* %4, i64 0, i64 0, i64 %551
  %592 = bitcast float* %591 to <8 x float>*
  store <8 x float> %590, <8 x float>* %592, align 16, !tbaa !136
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
  store <8 x float> %850, <8 x float>* %.sub, align 16, !tbaa !139
  store <8 x float> %856, <8 x float>* %228, align 16, !tbaa !139
  store <8 x float> %862, <8 x float>* %230, align 16, !tbaa !139
  store <8 x float> %868, <8 x float>* %232, align 16, !tbaa !139
  store <8 x float> %874, <8 x float>* %234, align 16, !tbaa !139
  store <8 x float> %880, <8 x float>* %236, align 16, !tbaa !139
  store <8 x float> %886, <8 x float>* %238, align 16, !tbaa !139
  store <8 x float> %892, <8 x float>* %240, align 16, !tbaa !139
  store <8 x float> %898, <8 x float>* %242, align 16, !tbaa !139
  store <8 x float> %904, <8 x float>* %244, align 16, !tbaa !139
  store <8 x float> %910, <8 x float>* %246, align 16, !tbaa !139
  store <8 x float> %916, <8 x float>* %248, align 16, !tbaa !139
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
  %627 = load float, float* %626, align 8, !tbaa !127
  %628 = insertelement <8 x float> undef, float %627, i32 0
  %629 = shufflevector <8 x float> %628, <8 x float> undef, <8 x i32> zeroinitializer
  %630 = shl i64 %indvars.iv, 3
  %631 = add nuw nsw i64 %609, %630
  %632 = getelementptr inbounds [110592 x <8 x float>], [110592 x <8 x float>]* %4, i64 0, i64 0, i64 %631
  %633 = bitcast float* %632 to <8 x float>*
  %634 = load <8 x float>, <8 x float>* %633, align 16, !tbaa !136
  %635 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %629, <8 x float> %634, <8 x float> %622)
  %636 = or i64 %624, 1
  %637 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %636
  %638 = load float, float* %637, align 4, !tbaa !127
  %639 = insertelement <8 x float> undef, float %638, i32 0
  %640 = shufflevector <8 x float> %639, <8 x float> undef, <8 x i32> zeroinitializer
  %641 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %640, <8 x float> %634, <8 x float> %621)
  %642 = add nuw nsw i64 %624, 2
  %643 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %642
  %644 = load float, float* %643, align 8, !tbaa !127
  %645 = insertelement <8 x float> undef, float %644, i32 0
  %646 = shufflevector <8 x float> %645, <8 x float> undef, <8 x i32> zeroinitializer
  %647 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %646, <8 x float> %634, <8 x float> %620)
  %648 = add nuw nsw i64 %624, 3
  %649 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %648
  %650 = load float, float* %649, align 4, !tbaa !127
  %651 = insertelement <8 x float> undef, float %650, i32 0
  %652 = shufflevector <8 x float> %651, <8 x float> undef, <8 x i32> zeroinitializer
  %653 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %652, <8 x float> %634, <8 x float> %619)
  %654 = add nuw nsw i64 %624, 4
  %655 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %654
  %656 = load float, float* %655, align 8, !tbaa !127
  %657 = insertelement <8 x float> undef, float %656, i32 0
  %658 = shufflevector <8 x float> %657, <8 x float> undef, <8 x i32> zeroinitializer
  %659 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %658, <8 x float> %634, <8 x float> %618)
  %660 = add nuw nsw i64 %624, 5
  %661 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %660
  %662 = load float, float* %661, align 4, !tbaa !127
  %663 = insertelement <8 x float> undef, float %662, i32 0
  %664 = shufflevector <8 x float> %663, <8 x float> undef, <8 x i32> zeroinitializer
  %665 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %664, <8 x float> %634, <8 x float> %617)
  %666 = add nuw nsw i64 %624, 6
  %667 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %666
  %668 = load float, float* %667, align 8, !tbaa !127
  %669 = insertelement <8 x float> undef, float %668, i32 0
  %670 = shufflevector <8 x float> %669, <8 x float> undef, <8 x i32> zeroinitializer
  %671 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %670, <8 x float> %634, <8 x float> %616)
  %672 = add nuw nsw i64 %624, 7
  %673 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %672
  %674 = load float, float* %673, align 4, !tbaa !127
  %675 = insertelement <8 x float> undef, float %674, i32 0
  %676 = shufflevector <8 x float> %675, <8 x float> undef, <8 x i32> zeroinitializer
  %677 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %676, <8 x float> %634, <8 x float> %615)
  %678 = add nuw nsw i64 %624, 8
  %679 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %678
  %680 = load float, float* %679, align 8, !tbaa !127
  %681 = insertelement <8 x float> undef, float %680, i32 0
  %682 = shufflevector <8 x float> %681, <8 x float> undef, <8 x i32> zeroinitializer
  %683 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %682, <8 x float> %634, <8 x float> %614)
  %684 = add nuw nsw i64 %624, 9
  %685 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %684
  %686 = load float, float* %685, align 4, !tbaa !127
  %687 = insertelement <8 x float> undef, float %686, i32 0
  %688 = shufflevector <8 x float> %687, <8 x float> undef, <8 x i32> zeroinitializer
  %689 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %688, <8 x float> %634, <8 x float> %613)
  %690 = add nuw nsw i64 %624, 10
  %691 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %690
  %692 = load float, float* %691, align 8, !tbaa !127
  %693 = insertelement <8 x float> undef, float %692, i32 0
  %694 = shufflevector <8 x float> %693, <8 x float> undef, <8 x i32> zeroinitializer
  %695 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %694, <8 x float> %634, <8 x float> %612)
  %696 = add nuw nsw i64 %624, 11
  %697 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %696
  %698 = load float, float* %697, align 4, !tbaa !127
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
  %715 = load <8 x float>, <8 x float>* %714, align 16, !tbaa !150
  %716 = getelementptr inbounds float, float* %249, i64 %704
  %717 = extractelement <8 x float> %715, i64 0
  store float %717, float* %716, align 4, !tbaa !151
  %718 = getelementptr inbounds float, float* %249, i64 %705
  %719 = extractelement <8 x float> %715, i64 1
  store float %719, float* %718, align 4, !tbaa !151
  %720 = getelementptr inbounds float, float* %249, i64 %706
  %721 = extractelement <8 x float> %715, i64 2
  store float %721, float* %720, align 4, !tbaa !151
  %722 = getelementptr inbounds float, float* %249, i64 %707
  %723 = extractelement <8 x float> %715, i64 3
  store float %723, float* %722, align 4, !tbaa !151
  %724 = getelementptr inbounds float, float* %249, i64 %708
  %725 = extractelement <8 x float> %715, i64 4
  store float %725, float* %724, align 4, !tbaa !151
  %726 = getelementptr inbounds float, float* %249, i64 %709
  %727 = extractelement <8 x float> %715, i64 5
  store float %727, float* %726, align 4, !tbaa !151
  %728 = getelementptr inbounds float, float* %249, i64 %710
  %729 = extractelement <8 x float> %715, i64 6
  store float %729, float* %728, align 4, !tbaa !151
  %730 = getelementptr inbounds float, float* %249, i64 %711
  %731 = extractelement <8 x float> %715, i64 7
  store float %731, float* %730, align 4, !tbaa !151
  %indvars.iv.next122 = add nuw nsw i64 %indvars.iv121, 1
  %exitcond123 = icmp eq i64 %indvars.iv.next122, 12
  br i1 %exitcond123, label %for_end36, label %for_body35, !prof !28

for_end36:                                        ; preds = %for_body35
  %732 = add nuw nsw i32 %593, 1
  %exitcond124 = icmp eq i32 %732, 576
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
  %749 = load float, float* %748, align 4, !tbaa !127
  %750 = insertelement <8 x float> undef, float %749, i32 0
  %751 = shufflevector <8 x float> %750, <8 x float> undef, <8 x i32> zeroinitializer
  %752 = shl i64 %indvars.iv.1, 3
  %753 = add nuw nsw i64 %703, %752
  %754 = getelementptr inbounds [110592 x <8 x float>], [110592 x <8 x float>]* %4, i64 0, i64 0, i64 %753
  %755 = bitcast float* %754 to <8 x float>*
  %756 = load <8 x float>, <8 x float>* %755, align 16, !tbaa !136
  %757 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %751, <8 x float> %756, <8 x float> %744)
  %758 = add nuw nsw i64 %746, 1
  %759 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %758
  %760 = load float, float* %759, align 8, !tbaa !127
  %761 = insertelement <8 x float> undef, float %760, i32 0
  %762 = shufflevector <8 x float> %761, <8 x float> undef, <8 x i32> zeroinitializer
  %763 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %762, <8 x float> %756, <8 x float> %743)
  %764 = add nuw nsw i64 %746, 2
  %765 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %764
  %766 = load float, float* %765, align 4, !tbaa !127
  %767 = insertelement <8 x float> undef, float %766, i32 0
  %768 = shufflevector <8 x float> %767, <8 x float> undef, <8 x i32> zeroinitializer
  %769 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %768, <8 x float> %756, <8 x float> %742)
  %770 = add nuw nsw i64 %746, 3
  %771 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %770
  %772 = load float, float* %771, align 8, !tbaa !127
  %773 = insertelement <8 x float> undef, float %772, i32 0
  %774 = shufflevector <8 x float> %773, <8 x float> undef, <8 x i32> zeroinitializer
  %775 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %774, <8 x float> %756, <8 x float> %741)
  %776 = add nuw nsw i64 %746, 4
  %777 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %776
  %778 = load float, float* %777, align 4, !tbaa !127
  %779 = insertelement <8 x float> undef, float %778, i32 0
  %780 = shufflevector <8 x float> %779, <8 x float> undef, <8 x i32> zeroinitializer
  %781 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %780, <8 x float> %756, <8 x float> %740)
  %782 = add nuw nsw i64 %746, 5
  %783 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %782
  %784 = load float, float* %783, align 8, !tbaa !127
  %785 = insertelement <8 x float> undef, float %784, i32 0
  %786 = shufflevector <8 x float> %785, <8 x float> undef, <8 x i32> zeroinitializer
  %787 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %786, <8 x float> %756, <8 x float> %739)
  %788 = add nuw nsw i64 %746, 6
  %789 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %788
  %790 = load float, float* %789, align 4, !tbaa !127
  %791 = insertelement <8 x float> undef, float %790, i32 0
  %792 = shufflevector <8 x float> %791, <8 x float> undef, <8 x i32> zeroinitializer
  %793 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %792, <8 x float> %756, <8 x float> %738)
  %794 = add nuw nsw i64 %746, 7
  %795 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %794
  %796 = load float, float* %795, align 8, !tbaa !127
  %797 = insertelement <8 x float> undef, float %796, i32 0
  %798 = shufflevector <8 x float> %797, <8 x float> undef, <8 x i32> zeroinitializer
  %799 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %798, <8 x float> %756, <8 x float> %737)
  %800 = add nuw nsw i64 %746, 8
  %801 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %800
  %802 = load float, float* %801, align 4, !tbaa !127
  %803 = insertelement <8 x float> undef, float %802, i32 0
  %804 = shufflevector <8 x float> %803, <8 x float> undef, <8 x i32> zeroinitializer
  %805 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %804, <8 x float> %756, <8 x float> %736)
  %806 = add nuw nsw i64 %746, 9
  %807 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %806
  %808 = load float, float* %807, align 8, !tbaa !127
  %809 = insertelement <8 x float> undef, float %808, i32 0
  %810 = shufflevector <8 x float> %809, <8 x float> undef, <8 x i32> zeroinitializer
  %811 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %810, <8 x float> %756, <8 x float> %735)
  %812 = add nuw nsw i64 %746, 10
  %813 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %812
  %814 = load float, float* %813, align 4, !tbaa !127
  %815 = insertelement <8 x float> undef, float %814, i32 0
  %816 = shufflevector <8 x float> %815, <8 x float> undef, <8 x i32> zeroinitializer
  %817 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %816, <8 x float> %756, <8 x float> %734)
  %818 = add nuw nsw i64 %746, 11
  %819 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %818
  %820 = load float, float* %819, align 8, !tbaa !127
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
  %842 = load float, float* %841, align 8, !tbaa !127
  %843 = insertelement <8 x float> undef, float %842, i32 0
  %844 = shufflevector <8 x float> %843, <8 x float> undef, <8 x i32> zeroinitializer
  %845 = shl i64 %indvars.iv.2, 3
  %846 = add nuw nsw i64 %825, %845
  %847 = getelementptr inbounds [110592 x <8 x float>], [110592 x <8 x float>]* %4, i64 0, i64 0, i64 %846
  %848 = bitcast float* %847 to <8 x float>*
  %849 = load <8 x float>, <8 x float>* %848, align 16, !tbaa !136
  %850 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %844, <8 x float> %849, <8 x float> %837)
  %851 = or i64 %839, 1
  %852 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %851
  %853 = load float, float* %852, align 4, !tbaa !127
  %854 = insertelement <8 x float> undef, float %853, i32 0
  %855 = shufflevector <8 x float> %854, <8 x float> undef, <8 x i32> zeroinitializer
  %856 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %855, <8 x float> %849, <8 x float> %836)
  %857 = add nuw nsw i64 %839, 2
  %858 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %857
  %859 = load float, float* %858, align 4, !tbaa !127
  %860 = insertelement <8 x float> undef, float %859, i32 0
  %861 = shufflevector <8 x float> %860, <8 x float> undef, <8 x i32> zeroinitializer
  %862 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %861, <8 x float> %849, <8 x float> %835)
  %863 = add nuw nsw i64 %839, 3
  %864 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %863
  %865 = load float, float* %864, align 4, !tbaa !127
  %866 = insertelement <8 x float> undef, float %865, i32 0
  %867 = shufflevector <8 x float> %866, <8 x float> undef, <8 x i32> zeroinitializer
  %868 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %867, <8 x float> %849, <8 x float> %834)
  %869 = add nuw nsw i64 %839, 4
  %870 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %869
  %871 = load float, float* %870, align 4, !tbaa !127
  %872 = insertelement <8 x float> undef, float %871, i32 0
  %873 = shufflevector <8 x float> %872, <8 x float> undef, <8 x i32> zeroinitializer
  %874 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %873, <8 x float> %849, <8 x float> %833)
  %875 = add nuw nsw i64 %839, 5
  %876 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %875
  %877 = load float, float* %876, align 4, !tbaa !127
  %878 = insertelement <8 x float> undef, float %877, i32 0
  %879 = shufflevector <8 x float> %878, <8 x float> undef, <8 x i32> zeroinitializer
  %880 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %879, <8 x float> %849, <8 x float> %832)
  %881 = add nuw nsw i64 %839, 6
  %882 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %881
  %883 = load float, float* %882, align 4, !tbaa !127
  %884 = insertelement <8 x float> undef, float %883, i32 0
  %885 = shufflevector <8 x float> %884, <8 x float> undef, <8 x i32> zeroinitializer
  %886 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %885, <8 x float> %849, <8 x float> %831)
  %887 = add nuw nsw i64 %839, 7
  %888 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %887
  %889 = load float, float* %888, align 4, !tbaa !127
  %890 = insertelement <8 x float> undef, float %889, i32 0
  %891 = shufflevector <8 x float> %890, <8 x float> undef, <8 x i32> zeroinitializer
  %892 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %891, <8 x float> %849, <8 x float> %830)
  %893 = add nuw nsw i64 %839, 8
  %894 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %893
  %895 = load float, float* %894, align 4, !tbaa !127
  %896 = insertelement <8 x float> undef, float %895, i32 0
  %897 = shufflevector <8 x float> %896, <8 x float> undef, <8 x i32> zeroinitializer
  %898 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %897, <8 x float> %849, <8 x float> %829)
  %899 = add nuw nsw i64 %839, 9
  %900 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %899
  %901 = load float, float* %900, align 4, !tbaa !127
  %902 = insertelement <8 x float> undef, float %901, i32 0
  %903 = shufflevector <8 x float> %902, <8 x float> undef, <8 x i32> zeroinitializer
  %904 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %903, <8 x float> %849, <8 x float> %828)
  %905 = add nuw nsw i64 %839, 10
  %906 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %905
  %907 = load float, float* %906, align 4, !tbaa !127
  %908 = insertelement <8 x float> undef, float %907, i32 0
  %909 = shufflevector <8 x float> %908, <8 x float> undef, <8 x i32> zeroinitializer
  %910 = tail call <8 x float> @llvm.fmuladd.v8f32(<8 x float> %909, <8 x float> %849, <8 x float> %827)
  %911 = add nuw nsw i64 %839, 11
  %912 = getelementptr inbounds [50176 x float], [50176 x float]* %5, i64 0, i64 %911
  %913 = load float, float* %912, align 4, !tbaa !127
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
  tail call fastcc void @fused_nn_conv2d_1_compute_(i8* %12, i8* %16, i8* %14)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_conv2d_1_compute_(i8* noalias nocapture readonly, i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #3 {
entry:
  %3 = alloca [75264 x float], align 16
  %4 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar17 = phi i64 [ 0, %entry ], [ %indvar.next18, %for_end3 ]
  %5 = mul nuw nsw i64 %indvar17, 196
  %6 = mul nuw nsw i64 %indvar17, 144
  %7 = add nsw i64 %6, -13
  br label %for_begin4.preheader

for_begin7.preheader:                             ; preds = %for_end3
  %8 = bitcast i8* %1 to float*
  %9 = bitcast i8* %2 to float*
  br label %for_begin10.preheader

for_begin4.preheader:                             ; preds = %for_end6, %for_begin1.preheader
  %indvar19 = phi i64 [ 0, %for_begin1.preheader ], [ %indvar.next20, %for_end6 ]
  %10 = mul nuw nsw i64 %indvar19, 14
  %11 = add nuw nsw i64 %5, %10
  %scevgep = getelementptr [75264 x float], [75264 x float]* %3, i64 0, i64 %11
  %12 = trunc i64 %indvar19 to i32
  %13 = add i32 %12, -1
  %14 = icmp ult i32 %13, 12
  %15 = mul nuw nsw i64 %indvar19, 12
  %16 = add nsw i64 %7, %15
  br i1 %14, label %if_end.us.13, label %for_body5.preheader

for_body5.preheader:                              ; preds = %for_begin4.preheader
  %scevgep21 = bitcast float* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %scevgep21, i8 0, i64 56, i1 false)
  br label %for_end6

for_end3:                                         ; preds = %for_end6
  %indvar.next18 = add nuw nsw i64 %indvar17, 1
  %exitcond27 = icmp eq i64 %indvar.next18, 384
  br i1 %exitcond27, label %for_begin7.preheader, label %for_begin1.preheader, !prof !28

for_end6:                                         ; preds = %for_body5.preheader, %if_end.us.13
  %indvar.next20 = add nuw nsw i64 %indvar19, 1
  %exitcond26 = icmp eq i64 %indvar.next20, 14
  br i1 %exitcond26, label %for_end3, label %for_begin4.preheader, !prof !28

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %indvar = phi i64 [ 0, %for_begin7.preheader ], [ %indvar.next, %for_end12 ]
  %17 = mul nuw nsw i64 %indvar, 144
  %18 = trunc i64 %indvar to i32
  %19 = udiv i32 %18, 192
  %20 = mul nsw i32 %19, 37632
  %21 = mul nuw nsw i64 %indvar, 1728
  %22 = zext i32 %20 to i64
  br label %for_begin13.preheader

for_end9:                                         ; preds = %for_end12
  ret void

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvar10 = phi i64 [ 0, %for_begin10.preheader ], [ %26, %for_end15 ]
  %23 = mul nuw nsw i64 %indvar10, 12
  %24 = add nuw nsw i64 %23, %17
  %25 = mul nuw nsw i64 %indvar10, 14
  %26 = add nuw nsw i64 %indvar10, 1
  %27 = mul nuw nsw i64 %26, 14
  %28 = mul i64 %indvar10, 14
  %29 = add i64 %28, 28
  br label %for_body14

for_end12:                                        ; preds = %for_end15
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond13 = icmp eq i64 %indvar.next, 384
  br i1 %exitcond13, label %for_end9, label %for_begin10.preheader, !prof !28

for_body14:                                       ; preds = %for_end18, %for_begin13.preheader
  %indvars.iv7 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next8, %for_end18 ]
  %30 = add nuw nsw i64 %24, %indvars.iv7
  %31 = getelementptr inbounds float, float* %8, i64 %30
  store float 0.000000e+00, float* %31, align 4, !tbaa !154
  %32 = add nuw nsw i64 %indvars.iv7, %22
  br label %for_begin19.preheader

for_end15:                                        ; preds = %for_end18
  %exitcond12 = icmp eq i64 %26, 12
  br i1 %exitcond12, label %for_end12, label %for_begin13.preheader, !prof !28

for_begin19.preheader:                            ; preds = %for_begin19.preheader, %for_body14
  %indvars.iv = phi i64 [ 0, %for_body14 ], [ %indvars.iv.next, %for_begin19.preheader ]
  %.lcssa.lcssa3 = phi float [ 0.000000e+00, %for_body14 ], [ %98, %for_begin19.preheader ]
  %33 = mul nuw nsw i64 %indvars.iv, 196
  %34 = add nuw nsw i64 %32, %33
  %35 = mul nuw nsw i64 %indvars.iv, 9
  %36 = add nuw nsw i64 %35, %21
  %37 = add nuw nsw i64 %34, %25
  %38 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %37
  %39 = load float, float* %38, align 4, !tbaa !157
  %40 = getelementptr inbounds float, float* %9, i64 %36
  %41 = load float, float* %40, align 4, !tbaa !160
  %42 = tail call float @llvm.fmuladd.f32(float %39, float %41, float %.lcssa.lcssa3)
  %43 = add nuw nsw i64 %37, 1
  %44 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %43
  %45 = load float, float* %44, align 4, !tbaa !157
  %46 = add nuw nsw i64 %36, 1
  %47 = getelementptr inbounds float, float* %9, i64 %46
  %48 = load float, float* %47, align 4, !tbaa !160
  %49 = tail call float @llvm.fmuladd.f32(float %45, float %48, float %42)
  %50 = add nuw nsw i64 %37, 2
  %51 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %50
  %52 = load float, float* %51, align 4, !tbaa !157
  %53 = add nuw nsw i64 %36, 2
  %54 = getelementptr inbounds float, float* %9, i64 %53
  %55 = load float, float* %54, align 4, !tbaa !160
  %56 = tail call float @llvm.fmuladd.f32(float %52, float %55, float %49)
  %57 = add nuw nsw i64 %34, %27
  %58 = add nuw nsw i64 %36, 3
  %59 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %57
  %60 = load float, float* %59, align 4, !tbaa !157
  %61 = getelementptr inbounds float, float* %9, i64 %58
  %62 = load float, float* %61, align 4, !tbaa !160
  %63 = tail call float @llvm.fmuladd.f32(float %60, float %62, float %56)
  %64 = add nuw nsw i64 %57, 1
  %65 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %64
  %66 = load float, float* %65, align 4, !tbaa !157
  %67 = add nuw nsw i64 %36, 4
  %68 = getelementptr inbounds float, float* %9, i64 %67
  %69 = load float, float* %68, align 4, !tbaa !160
  %70 = tail call float @llvm.fmuladd.f32(float %66, float %69, float %63)
  %71 = add nuw nsw i64 %57, 2
  %72 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %71
  %73 = load float, float* %72, align 4, !tbaa !157
  %74 = add nuw nsw i64 %36, 5
  %75 = getelementptr inbounds float, float* %9, i64 %74
  %76 = load float, float* %75, align 4, !tbaa !160
  %77 = tail call float @llvm.fmuladd.f32(float %73, float %76, float %70)
  %78 = add nuw nsw i64 %34, %29
  %79 = add nuw nsw i64 %36, 6
  %80 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %78
  %81 = load float, float* %80, align 4, !tbaa !157
  %82 = getelementptr inbounds float, float* %9, i64 %79
  %83 = load float, float* %82, align 4, !tbaa !160
  %84 = tail call float @llvm.fmuladd.f32(float %81, float %83, float %77)
  %85 = add nuw nsw i64 %78, 1
  %86 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %85
  %87 = load float, float* %86, align 4, !tbaa !157
  %88 = add nuw nsw i64 %36, 7
  %89 = getelementptr inbounds float, float* %9, i64 %88
  %90 = load float, float* %89, align 4, !tbaa !160
  %91 = tail call float @llvm.fmuladd.f32(float %87, float %90, float %84)
  %92 = add nuw nsw i64 %78, 2
  %93 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %92
  %94 = load float, float* %93, align 4, !tbaa !157
  %95 = add nuw nsw i64 %36, 8
  %96 = getelementptr inbounds float, float* %9, i64 %95
  %97 = load float, float* %96, align 4, !tbaa !160
  %98 = tail call float @llvm.fmuladd.f32(float %94, float %97, float %91)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 192
  br i1 %exitcond, label %for_end18, label %for_begin19.preheader, !prof !28

for_end18:                                        ; preds = %for_begin19.preheader
  store float %98, float* %31, align 4, !tbaa !154
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  %exitcond9 = icmp eq i64 %indvars.iv.next8, 12
  br i1 %exitcond9, label %for_end15, label %for_body14, !prof !28

if_end.us.13:                                     ; preds = %for_begin4.preheader
  store float 0.000000e+00, float* %scevgep, align 8, !tbaa !157
  %99 = or i64 %11, 1
  %100 = add nsw i64 %16, 1
  %101 = getelementptr inbounds float, float* %4, i64 %100
  %102 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %99
  %103 = bitcast float* %101 to <4 x i32>*
  %104 = load <4 x i32>, <4 x i32>* %103, align 4, !tbaa !163
  %105 = bitcast float* %102 to <4 x i32>*
  store <4 x i32> %104, <4 x i32>* %105, align 4, !tbaa !157
  %106 = add nuw nsw i64 %11, 5
  %107 = add nsw i64 %16, 5
  %108 = getelementptr inbounds float, float* %4, i64 %107
  %109 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %106
  %110 = bitcast float* %108 to <4 x i32>*
  %111 = load <4 x i32>, <4 x i32>* %110, align 4, !tbaa !163
  %112 = bitcast float* %109 to <4 x i32>*
  store <4 x i32> %111, <4 x i32>* %112, align 4, !tbaa !157
  %113 = add nuw nsw i64 %11, 9
  %114 = add nsw i64 %16, 9
  %115 = getelementptr inbounds float, float* %4, i64 %114
  %116 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %113
  %117 = bitcast float* %115 to <4 x i32>*
  %118 = load <4 x i32>, <4 x i32>* %117, align 4, !tbaa !163
  %119 = bitcast float* %116 to <4 x i32>*
  store <4 x i32> %118, <4 x i32>* %119, align 4, !tbaa !157
  %120 = add nuw nsw i64 %11, 13
  %121 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %120
  store float 0.000000e+00, float* %121, align 4, !tbaa !157
  br label %for_end6
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
  %8 = load float, float* %7, align 4, !tbaa !166
  %9 = insertelement <4 x float> undef, float %8, i32 0
  %10 = shufflevector <4 x float> %9, <4 x float> undef, <4 x i32> zeroinitializer
  %11 = insertelement <4 x float> undef, float %8, i32 0
  %12 = shufflevector <4 x float> %11, <4 x float> undef, <4 x i32> zeroinitializer
  %13 = insertelement <4 x float> undef, float %8, i32 0
  %14 = shufflevector <4 x float> %13, <4 x float> undef, <4 x i32> zeroinitializer
  %15 = getelementptr inbounds float, float* %4, i64 %6
  %16 = getelementptr inbounds float, float* %5, i64 %6
  %17 = bitcast float* %15 to <4 x float>*
  %18 = load <4 x float>, <4 x float>* %17, align 4, !tbaa !169
  %19 = fadd <4 x float> %10, %18
  %20 = bitcast float* %16 to <4 x float>*
  store <4 x float> %19, <4 x float>* %20, align 4, !tbaa !172
  %21 = or i64 %6, 4
  %22 = getelementptr inbounds float, float* %4, i64 %21
  %23 = getelementptr inbounds float, float* %5, i64 %21
  %24 = bitcast float* %22 to <4 x float>*
  %25 = load <4 x float>, <4 x float>* %24, align 4, !tbaa !169
  %26 = fadd <4 x float> %12, %25
  %27 = bitcast float* %23 to <4 x float>*
  store <4 x float> %26, <4 x float>* %27, align 4, !tbaa !172
  %28 = or i64 %6, 8
  %29 = getelementptr inbounds float, float* %4, i64 %28
  %30 = getelementptr inbounds float, float* %5, i64 %28
  %31 = bitcast float* %29 to <4 x float>*
  %32 = load <4 x float>, <4 x float>* %31, align 4, !tbaa !169
  %33 = fadd <4 x float> %14, %32
  %34 = bitcast float* %30 to <4 x float>*
  store <4 x float> %33, <4 x float>* %34, align 4, !tbaa !172
  %35 = or i64 %6, 12
  %36 = getelementptr inbounds float, float* %4, i64 %35
  %37 = getelementptr inbounds float, float* %5, i64 %35
  %38 = bitcast float* %36 to <4 x float>*
  %39 = load <4 x float>, <4 x float>* %38, align 4, !tbaa !169
  %40 = fadd <4 x float> %10, %39
  %41 = bitcast float* %37 to <4 x float>*
  store <4 x float> %40, <4 x float>* %41, align 4, !tbaa !172
  %42 = add nuw nsw i64 %35, 4
  %43 = getelementptr inbounds float, float* %4, i64 %42
  %44 = getelementptr inbounds float, float* %5, i64 %42
  %45 = bitcast float* %43 to <4 x float>*
  %46 = load <4 x float>, <4 x float>* %45, align 4, !tbaa !169
  %47 = fadd <4 x float> %12, %46
  %48 = bitcast float* %44 to <4 x float>*
  store <4 x float> %47, <4 x float>* %48, align 4, !tbaa !172
  %49 = add nuw nsw i64 %35, 8
  %50 = getelementptr inbounds float, float* %4, i64 %49
  %51 = getelementptr inbounds float, float* %5, i64 %49
  %52 = bitcast float* %50 to <4 x float>*
  %53 = load <4 x float>, <4 x float>* %52, align 4, !tbaa !169
  %54 = fadd <4 x float> %14, %53
  %55 = bitcast float* %51 to <4 x float>*
  store <4 x float> %54, <4 x float>* %55, align 4, !tbaa !172
  %56 = add nuw nsw i64 %6, 24
  %57 = getelementptr inbounds float, float* %4, i64 %56
  %58 = getelementptr inbounds float, float* %5, i64 %56
  %59 = bitcast float* %57 to <4 x float>*
  %60 = load <4 x float>, <4 x float>* %59, align 4, !tbaa !169
  %61 = fadd <4 x float> %10, %60
  %62 = bitcast float* %58 to <4 x float>*
  store <4 x float> %61, <4 x float>* %62, align 4, !tbaa !172
  %63 = add nuw nsw i64 %6, 28
  %64 = getelementptr inbounds float, float* %4, i64 %63
  %65 = getelementptr inbounds float, float* %5, i64 %63
  %66 = bitcast float* %64 to <4 x float>*
  %67 = load <4 x float>, <4 x float>* %66, align 4, !tbaa !169
  %68 = fadd <4 x float> %12, %67
  %69 = bitcast float* %65 to <4 x float>*
  store <4 x float> %68, <4 x float>* %69, align 4, !tbaa !172
  %70 = add nuw nsw i64 %6, 32
  %71 = getelementptr inbounds float, float* %4, i64 %70
  %72 = getelementptr inbounds float, float* %5, i64 %70
  %73 = bitcast float* %71 to <4 x float>*
  %74 = load <4 x float>, <4 x float>* %73, align 4, !tbaa !169
  %75 = fadd <4 x float> %14, %74
  %76 = bitcast float* %72 to <4 x float>*
  store <4 x float> %75, <4 x float>* %76, align 4, !tbaa !172
  %77 = add nuw nsw i64 %6, 36
  %78 = getelementptr inbounds float, float* %4, i64 %77
  %79 = getelementptr inbounds float, float* %5, i64 %77
  %80 = bitcast float* %78 to <4 x float>*
  %81 = load <4 x float>, <4 x float>* %80, align 4, !tbaa !169
  %82 = fadd <4 x float> %10, %81
  %83 = bitcast float* %79 to <4 x float>*
  store <4 x float> %82, <4 x float>* %83, align 4, !tbaa !172
  %84 = add nuw nsw i64 %6, 40
  %85 = getelementptr inbounds float, float* %4, i64 %84
  %86 = getelementptr inbounds float, float* %5, i64 %84
  %87 = bitcast float* %85 to <4 x float>*
  %88 = load <4 x float>, <4 x float>* %87, align 4, !tbaa !169
  %89 = fadd <4 x float> %12, %88
  %90 = bitcast float* %86 to <4 x float>*
  store <4 x float> %89, <4 x float>* %90, align 4, !tbaa !172
  %91 = add nuw nsw i64 %6, 44
  %92 = getelementptr inbounds float, float* %4, i64 %91
  %93 = getelementptr inbounds float, float* %5, i64 %91
  %94 = bitcast float* %92 to <4 x float>*
  %95 = load <4 x float>, <4 x float>* %94, align 4, !tbaa !169
  %96 = fadd <4 x float> %14, %95
  %97 = bitcast float* %93 to <4 x float>*
  store <4 x float> %96, <4 x float>* %97, align 4, !tbaa !172
  %98 = add nuw nsw i64 %6, 48
  %99 = getelementptr inbounds float, float* %4, i64 %98
  %100 = getelementptr inbounds float, float* %5, i64 %98
  %101 = bitcast float* %99 to <4 x float>*
  %102 = load <4 x float>, <4 x float>* %101, align 4, !tbaa !169
  %103 = fadd <4 x float> %10, %102
  %104 = bitcast float* %100 to <4 x float>*
  store <4 x float> %103, <4 x float>* %104, align 4, !tbaa !172
  %105 = add nuw nsw i64 %6, 52
  %106 = getelementptr inbounds float, float* %4, i64 %105
  %107 = getelementptr inbounds float, float* %5, i64 %105
  %108 = bitcast float* %106 to <4 x float>*
  %109 = load <4 x float>, <4 x float>* %108, align 4, !tbaa !169
  %110 = fadd <4 x float> %12, %109
  %111 = bitcast float* %107 to <4 x float>*
  store <4 x float> %110, <4 x float>* %111, align 4, !tbaa !172
  %112 = add nuw nsw i64 %6, 56
  %113 = getelementptr inbounds float, float* %4, i64 %112
  %114 = getelementptr inbounds float, float* %5, i64 %112
  %115 = bitcast float* %113 to <4 x float>*
  %116 = load <4 x float>, <4 x float>* %115, align 4, !tbaa !169
  %117 = fadd <4 x float> %14, %116
  %118 = bitcast float* %114 to <4 x float>*
  store <4 x float> %117, <4 x float>* %118, align 4, !tbaa !172
  %119 = add nuw nsw i64 %6, 60
  %120 = getelementptr inbounds float, float* %4, i64 %119
  %121 = getelementptr inbounds float, float* %5, i64 %119
  %122 = bitcast float* %120 to <4 x float>*
  %123 = load <4 x float>, <4 x float>* %122, align 4, !tbaa !169
  %124 = fadd <4 x float> %10, %123
  %125 = bitcast float* %121 to <4 x float>*
  store <4 x float> %124, <4 x float>* %125, align 4, !tbaa !172
  %126 = add nuw nsw i64 %6, 64
  %127 = getelementptr inbounds float, float* %4, i64 %126
  %128 = getelementptr inbounds float, float* %5, i64 %126
  %129 = bitcast float* %127 to <4 x float>*
  %130 = load <4 x float>, <4 x float>* %129, align 4, !tbaa !169
  %131 = fadd <4 x float> %12, %130
  %132 = bitcast float* %128 to <4 x float>*
  store <4 x float> %131, <4 x float>* %132, align 4, !tbaa !172
  %133 = add nuw nsw i64 %6, 68
  %134 = getelementptr inbounds float, float* %4, i64 %133
  %135 = getelementptr inbounds float, float* %5, i64 %133
  %136 = bitcast float* %134 to <4 x float>*
  %137 = load <4 x float>, <4 x float>* %136, align 4, !tbaa !169
  %138 = fadd <4 x float> %14, %137
  %139 = bitcast float* %135 to <4 x float>*
  store <4 x float> %138, <4 x float>* %139, align 4, !tbaa !172
  %140 = add nuw nsw i64 %6, 72
  %141 = getelementptr inbounds float, float* %4, i64 %140
  %142 = getelementptr inbounds float, float* %5, i64 %140
  %143 = bitcast float* %141 to <4 x float>*
  %144 = load <4 x float>, <4 x float>* %143, align 4, !tbaa !169
  %145 = fadd <4 x float> %10, %144
  %146 = bitcast float* %142 to <4 x float>*
  store <4 x float> %145, <4 x float>* %146, align 4, !tbaa !172
  %147 = add nuw nsw i64 %6, 76
  %148 = getelementptr inbounds float, float* %4, i64 %147
  %149 = getelementptr inbounds float, float* %5, i64 %147
  %150 = bitcast float* %148 to <4 x float>*
  %151 = load <4 x float>, <4 x float>* %150, align 4, !tbaa !169
  %152 = fadd <4 x float> %12, %151
  %153 = bitcast float* %149 to <4 x float>*
  store <4 x float> %152, <4 x float>* %153, align 4, !tbaa !172
  %154 = add nuw nsw i64 %6, 80
  %155 = getelementptr inbounds float, float* %4, i64 %154
  %156 = getelementptr inbounds float, float* %5, i64 %154
  %157 = bitcast float* %155 to <4 x float>*
  %158 = load <4 x float>, <4 x float>* %157, align 4, !tbaa !169
  %159 = fadd <4 x float> %14, %158
  %160 = bitcast float* %156 to <4 x float>*
  store <4 x float> %159, <4 x float>* %160, align 4, !tbaa !172
  %161 = add nuw nsw i64 %6, 84
  %162 = getelementptr inbounds float, float* %4, i64 %161
  %163 = getelementptr inbounds float, float* %5, i64 %161
  %164 = bitcast float* %162 to <4 x float>*
  %165 = load <4 x float>, <4 x float>* %164, align 4, !tbaa !169
  %166 = fadd <4 x float> %10, %165
  %167 = bitcast float* %163 to <4 x float>*
  store <4 x float> %166, <4 x float>* %167, align 4, !tbaa !172
  %168 = add nuw nsw i64 %6, 88
  %169 = getelementptr inbounds float, float* %4, i64 %168
  %170 = getelementptr inbounds float, float* %5, i64 %168
  %171 = bitcast float* %169 to <4 x float>*
  %172 = load <4 x float>, <4 x float>* %171, align 4, !tbaa !169
  %173 = fadd <4 x float> %12, %172
  %174 = bitcast float* %170 to <4 x float>*
  store <4 x float> %173, <4 x float>* %174, align 4, !tbaa !172
  %175 = add nuw nsw i64 %6, 92
  %176 = getelementptr inbounds float, float* %4, i64 %175
  %177 = getelementptr inbounds float, float* %5, i64 %175
  %178 = bitcast float* %176 to <4 x float>*
  %179 = load <4 x float>, <4 x float>* %178, align 4, !tbaa !169
  %180 = fadd <4 x float> %14, %179
  %181 = bitcast float* %177 to <4 x float>*
  store <4 x float> %180, <4 x float>* %181, align 4, !tbaa !172
  %182 = add nuw nsw i64 %6, 96
  %183 = getelementptr inbounds float, float* %4, i64 %182
  %184 = getelementptr inbounds float, float* %5, i64 %182
  %185 = bitcast float* %183 to <4 x float>*
  %186 = load <4 x float>, <4 x float>* %185, align 4, !tbaa !169
  %187 = fadd <4 x float> %10, %186
  %188 = bitcast float* %184 to <4 x float>*
  store <4 x float> %187, <4 x float>* %188, align 4, !tbaa !172
  %189 = add nuw nsw i64 %6, 100
  %190 = getelementptr inbounds float, float* %4, i64 %189
  %191 = getelementptr inbounds float, float* %5, i64 %189
  %192 = bitcast float* %190 to <4 x float>*
  %193 = load <4 x float>, <4 x float>* %192, align 4, !tbaa !169
  %194 = fadd <4 x float> %12, %193
  %195 = bitcast float* %191 to <4 x float>*
  store <4 x float> %194, <4 x float>* %195, align 4, !tbaa !172
  %196 = add nuw nsw i64 %6, 104
  %197 = getelementptr inbounds float, float* %4, i64 %196
  %198 = getelementptr inbounds float, float* %5, i64 %196
  %199 = bitcast float* %197 to <4 x float>*
  %200 = load <4 x float>, <4 x float>* %199, align 4, !tbaa !169
  %201 = fadd <4 x float> %14, %200
  %202 = bitcast float* %198 to <4 x float>*
  store <4 x float> %201, <4 x float>* %202, align 4, !tbaa !172
  %203 = add nuw nsw i64 %6, 108
  %204 = getelementptr inbounds float, float* %4, i64 %203
  %205 = getelementptr inbounds float, float* %5, i64 %203
  %206 = bitcast float* %204 to <4 x float>*
  %207 = load <4 x float>, <4 x float>* %206, align 4, !tbaa !169
  %208 = fadd <4 x float> %10, %207
  %209 = bitcast float* %205 to <4 x float>*
  store <4 x float> %208, <4 x float>* %209, align 4, !tbaa !172
  %210 = add nuw nsw i64 %6, 112
  %211 = getelementptr inbounds float, float* %4, i64 %210
  %212 = getelementptr inbounds float, float* %5, i64 %210
  %213 = bitcast float* %211 to <4 x float>*
  %214 = load <4 x float>, <4 x float>* %213, align 4, !tbaa !169
  %215 = fadd <4 x float> %12, %214
  %216 = bitcast float* %212 to <4 x float>*
  store <4 x float> %215, <4 x float>* %216, align 4, !tbaa !172
  %217 = add nuw nsw i64 %6, 116
  %218 = getelementptr inbounds float, float* %4, i64 %217
  %219 = getelementptr inbounds float, float* %5, i64 %217
  %220 = bitcast float* %218 to <4 x float>*
  %221 = load <4 x float>, <4 x float>* %220, align 4, !tbaa !169
  %222 = fadd <4 x float> %14, %221
  %223 = bitcast float* %219 to <4 x float>*
  store <4 x float> %222, <4 x float>* %223, align 4, !tbaa !172
  %224 = add nuw nsw i64 %6, 120
  %225 = getelementptr inbounds float, float* %4, i64 %224
  %226 = getelementptr inbounds float, float* %5, i64 %224
  %227 = bitcast float* %225 to <4 x float>*
  %228 = load <4 x float>, <4 x float>* %227, align 4, !tbaa !169
  %229 = fadd <4 x float> %10, %228
  %230 = bitcast float* %226 to <4 x float>*
  store <4 x float> %229, <4 x float>* %230, align 4, !tbaa !172
  %231 = add nuw nsw i64 %6, 124
  %232 = getelementptr inbounds float, float* %4, i64 %231
  %233 = getelementptr inbounds float, float* %5, i64 %231
  %234 = bitcast float* %232 to <4 x float>*
  %235 = load <4 x float>, <4 x float>* %234, align 4, !tbaa !169
  %236 = fadd <4 x float> %12, %235
  %237 = bitcast float* %233 to <4 x float>*
  store <4 x float> %236, <4 x float>* %237, align 4, !tbaa !172
  %238 = add nuw nsw i64 %6, 128
  %239 = getelementptr inbounds float, float* %4, i64 %238
  %240 = getelementptr inbounds float, float* %5, i64 %238
  %241 = bitcast float* %239 to <4 x float>*
  %242 = load <4 x float>, <4 x float>* %241, align 4, !tbaa !169
  %243 = fadd <4 x float> %14, %242
  %244 = bitcast float* %240 to <4 x float>*
  store <4 x float> %243, <4 x float>* %244, align 4, !tbaa !172
  %245 = add nuw nsw i64 %6, 132
  %246 = getelementptr inbounds float, float* %4, i64 %245
  %247 = getelementptr inbounds float, float* %5, i64 %245
  %248 = bitcast float* %246 to <4 x float>*
  %249 = load <4 x float>, <4 x float>* %248, align 4, !tbaa !169
  %250 = fadd <4 x float> %10, %249
  %251 = bitcast float* %247 to <4 x float>*
  store <4 x float> %250, <4 x float>* %251, align 4, !tbaa !172
  %252 = add nuw nsw i64 %6, 136
  %253 = getelementptr inbounds float, float* %4, i64 %252
  %254 = getelementptr inbounds float, float* %5, i64 %252
  %255 = bitcast float* %253 to <4 x float>*
  %256 = load <4 x float>, <4 x float>* %255, align 4, !tbaa !169
  %257 = fadd <4 x float> %12, %256
  %258 = bitcast float* %254 to <4 x float>*
  store <4 x float> %257, <4 x float>* %258, align 4, !tbaa !172
  %259 = add nuw nsw i64 %6, 140
  %260 = getelementptr inbounds float, float* %4, i64 %259
  %261 = getelementptr inbounds float, float* %5, i64 %259
  %262 = bitcast float* %260 to <4 x float>*
  %263 = load <4 x float>, <4 x float>* %262, align 4, !tbaa !169
  %264 = fadd <4 x float> %14, %263
  %265 = bitcast float* %261 to <4 x float>*
  store <4 x float> %264, <4 x float>* %265, align 4, !tbaa !172
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 384
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !28

for_end:                                          ; preds = %for_begin1.preheader
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
  %10 = load <16 x float>, <16 x float>* %9, align 64, !tbaa !175
  %11 = add nuw nsw i64 %7, %6
  %12 = getelementptr inbounds float, float* %4, i64 %11
  %13 = bitcast float* %12 to <16 x float>*
  %14 = load <16 x float>, <16 x float>* %13, align 64, !tbaa !178
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
  store float %32, float* %16, align 4, !tbaa !181
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 4096
  br i1 %exitcond5, label %for_end, label %for_begin1.preheader, !prof !28
}

; Function Attrs: nounwind readnone speculatable
declare <16 x float> @llvm.fmuladd.v16f32(<16 x float>, <16 x float>, <16 x float>) #4

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
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 16384, i1 false)
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
  %10 = load <16 x float>, <16 x float>* %9, align 64, !tbaa !184
  %11 = add nuw nsw i64 %7, %6
  %12 = getelementptr inbounds float, float* %4, i64 %11
  %13 = bitcast float* %12 to <16 x float>*
  %14 = load <16 x float>, <16 x float>* %13, align 64, !tbaa !187
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
  store float %32, float* %16, align 4, !tbaa !190
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 1000
  br i1 %exitcond5, label %for_end, label %for_begin1.preheader, !prof !28
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
  %5 = mul nuw nsw i64 %indvars.iv6, 676
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv3 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next4, %for_begin4.preheader ]
  %6 = mul nuw nsw i64 %indvars.iv3, 12
  %7 = add nuw nsw i64 %6, %4
  %8 = mul nuw nsw i64 %indvars.iv3, 52
  %9 = add nuw nsw i64 %8, %5
  %10 = getelementptr inbounds float, float* %2, i64 %7
  %11 = getelementptr inbounds float, float* %3, i64 %9
  %12 = bitcast float* %11 to <8 x float>*
  %wide.vec = load <8 x float>, <8 x float>* %12, align 4, !tbaa !193
  %strided.vec = shufflevector <8 x float> %wide.vec, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %13 = fcmp olt <4 x float> %strided.vec, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %14 = select <4 x i1> %13, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec
  %15 = or i64 %9, 1
  %16 = getelementptr inbounds float, float* %3, i64 %15
  %17 = bitcast float* %16 to <8 x float>*
  %wide.vec9 = load <8 x float>, <8 x float>* %17, align 4, !tbaa !193
  %strided.vec10 = shufflevector <8 x float> %wide.vec9, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec11 = shufflevector <8 x float> %wide.vec9, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %18 = fcmp ogt <4 x float> %14, %strided.vec10
  %19 = select <4 x i1> %18, <4 x float> %14, <4 x float> %strided.vec10
  %20 = fcmp ogt <4 x float> %19, %strided.vec11
  %21 = select <4 x i1> %20, <4 x float> %19, <4 x float> %strided.vec11
  %22 = add nuw nsw i64 %9, 26
  %23 = getelementptr inbounds float, float* %3, i64 %22
  %24 = bitcast float* %23 to <8 x float>*
  %wide.vec12 = load <8 x float>, <8 x float>* %24, align 4, !tbaa !193
  %strided.vec13 = shufflevector <8 x float> %wide.vec12, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %25 = fcmp ogt <4 x float> %21, %strided.vec13
  %26 = select <4 x i1> %25, <4 x float> %21, <4 x float> %strided.vec13
  %27 = add nuw nsw i64 %9, 27
  %28 = getelementptr inbounds float, float* %3, i64 %27
  %29 = bitcast float* %28 to <8 x float>*
  %wide.vec14 = load <8 x float>, <8 x float>* %29, align 4, !tbaa !193
  %strided.vec15 = shufflevector <8 x float> %wide.vec14, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec16 = shufflevector <8 x float> %wide.vec14, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %30 = fcmp ogt <4 x float> %26, %strided.vec15
  %31 = select <4 x i1> %30, <4 x float> %26, <4 x float> %strided.vec15
  %32 = fcmp ogt <4 x float> %31, %strided.vec16
  %33 = select <4 x i1> %32, <4 x float> %31, <4 x float> %strided.vec16
  %34 = add nuw nsw i64 %9, 52
  %35 = getelementptr inbounds float, float* %3, i64 %34
  %36 = bitcast float* %35 to <8 x float>*
  %wide.vec17 = load <8 x float>, <8 x float>* %36, align 4, !tbaa !193
  %strided.vec18 = shufflevector <8 x float> %wide.vec17, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %37 = fcmp ogt <4 x float> %33, %strided.vec18
  %38 = select <4 x i1> %37, <4 x float> %33, <4 x float> %strided.vec18
  %39 = add nuw nsw i64 %9, 53
  %40 = getelementptr inbounds float, float* %3, i64 %39
  %41 = bitcast float* %40 to <8 x float>*
  %wide.vec19 = load <8 x float>, <8 x float>* %41, align 4, !tbaa !193
  %strided.vec20 = shufflevector <8 x float> %wide.vec19, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec21 = shufflevector <8 x float> %wide.vec19, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %42 = fcmp ogt <4 x float> %38, %strided.vec20
  %43 = select <4 x i1> %42, <4 x float> %38, <4 x float> %strided.vec20
  %44 = fcmp ogt <4 x float> %43, %strided.vec21
  %45 = select <4 x i1> %44, <4 x float> %43, <4 x float> %strided.vec21
  %46 = bitcast float* %10 to <4 x float>*
  store <4 x float> %45, <4 x float>* %46, align 4, !tbaa !196
  %47 = add nuw nsw i64 %7, 4
  %48 = getelementptr inbounds float, float* %2, i64 %47
  %49 = add nuw nsw i64 %9, 8
  %50 = getelementptr inbounds float, float* %3, i64 %49
  %51 = bitcast float* %50 to <8 x float>*
  %wide.vec.1 = load <8 x float>, <8 x float>* %51, align 4, !tbaa !193
  %strided.vec.1 = shufflevector <8 x float> %wide.vec.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %52 = fcmp olt <4 x float> %strided.vec.1, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %53 = select <4 x i1> %52, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.1
  %54 = or i64 %49, 1
  %55 = getelementptr inbounds float, float* %3, i64 %54
  %56 = bitcast float* %55 to <8 x float>*
  %wide.vec9.1 = load <8 x float>, <8 x float>* %56, align 4, !tbaa !193
  %strided.vec10.1 = shufflevector <8 x float> %wide.vec9.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec11.1 = shufflevector <8 x float> %wide.vec9.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %57 = fcmp ogt <4 x float> %53, %strided.vec10.1
  %58 = select <4 x i1> %57, <4 x float> %53, <4 x float> %strided.vec10.1
  %59 = fcmp ogt <4 x float> %58, %strided.vec11.1
  %60 = select <4 x i1> %59, <4 x float> %58, <4 x float> %strided.vec11.1
  %61 = add nuw nsw i64 %9, 34
  %62 = getelementptr inbounds float, float* %3, i64 %61
  %63 = bitcast float* %62 to <8 x float>*
  %wide.vec12.1 = load <8 x float>, <8 x float>* %63, align 4, !tbaa !193
  %strided.vec13.1 = shufflevector <8 x float> %wide.vec12.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %64 = fcmp ogt <4 x float> %60, %strided.vec13.1
  %65 = select <4 x i1> %64, <4 x float> %60, <4 x float> %strided.vec13.1
  %66 = add nuw nsw i64 %9, 35
  %67 = getelementptr inbounds float, float* %3, i64 %66
  %68 = bitcast float* %67 to <8 x float>*
  %wide.vec14.1 = load <8 x float>, <8 x float>* %68, align 4, !tbaa !193
  %strided.vec15.1 = shufflevector <8 x float> %wide.vec14.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec16.1 = shufflevector <8 x float> %wide.vec14.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %69 = fcmp ogt <4 x float> %65, %strided.vec15.1
  %70 = select <4 x i1> %69, <4 x float> %65, <4 x float> %strided.vec15.1
  %71 = fcmp ogt <4 x float> %70, %strided.vec16.1
  %72 = select <4 x i1> %71, <4 x float> %70, <4 x float> %strided.vec16.1
  %73 = add nuw nsw i64 %9, 60
  %74 = getelementptr inbounds float, float* %3, i64 %73
  %75 = bitcast float* %74 to <8 x float>*
  %wide.vec17.1 = load <8 x float>, <8 x float>* %75, align 4, !tbaa !193
  %strided.vec18.1 = shufflevector <8 x float> %wide.vec17.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %76 = fcmp ogt <4 x float> %72, %strided.vec18.1
  %77 = select <4 x i1> %76, <4 x float> %72, <4 x float> %strided.vec18.1
  %78 = add nuw nsw i64 %9, 61
  %79 = getelementptr inbounds float, float* %3, i64 %78
  %80 = bitcast float* %79 to <8 x float>*
  %wide.vec19.1 = load <8 x float>, <8 x float>* %80, align 4, !tbaa !193
  %strided.vec20.1 = shufflevector <8 x float> %wide.vec19.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec21.1 = shufflevector <8 x float> %wide.vec19.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %81 = fcmp ogt <4 x float> %77, %strided.vec20.1
  %82 = select <4 x i1> %81, <4 x float> %77, <4 x float> %strided.vec20.1
  %83 = fcmp ogt <4 x float> %82, %strided.vec21.1
  %84 = select <4 x i1> %83, <4 x float> %82, <4 x float> %strided.vec21.1
  %85 = bitcast float* %48 to <4 x float>*
  store <4 x float> %84, <4 x float>* %85, align 4, !tbaa !196
  %86 = add nuw nsw i64 %7, 8
  %87 = getelementptr inbounds float, float* %2, i64 %86
  %88 = add nuw nsw i64 %9, 16
  %89 = getelementptr inbounds float, float* %3, i64 %88
  %90 = load float, float* %89, align 4, !tbaa !193
  %91 = fcmp olt float %90, 0xC7EFFFFFE0000000
  %92 = select i1 %91, float 0xC7EFFFFFE0000000, float %90
  %93 = or i64 %88, 1
  %94 = getelementptr inbounds float, float* %3, i64 %93
  %95 = load float, float* %94, align 4, !tbaa !193
  %96 = fcmp ogt float %92, %95
  %97 = select i1 %96, float %92, float %95
  %98 = add nuw nsw i64 %9, 18
  %99 = getelementptr inbounds float, float* %3, i64 %98
  %100 = load float, float* %99, align 4, !tbaa !193
  %101 = fcmp ogt float %97, %100
  %102 = select i1 %101, float %97, float %100
  %103 = add nuw nsw i64 %9, 42
  %104 = getelementptr inbounds float, float* %3, i64 %103
  %105 = load float, float* %104, align 4, !tbaa !193
  %106 = fcmp ogt float %102, %105
  %107 = select i1 %106, float %102, float %105
  %108 = add nuw nsw i64 %9, 43
  %109 = getelementptr inbounds float, float* %3, i64 %108
  %110 = load float, float* %109, align 4, !tbaa !193
  %111 = fcmp ogt float %107, %110
  %112 = select i1 %111, float %107, float %110
  %113 = add nuw nsw i64 %9, 44
  %114 = getelementptr inbounds float, float* %3, i64 %113
  %115 = load float, float* %114, align 4, !tbaa !193
  %116 = fcmp ogt float %112, %115
  %117 = select i1 %116, float %112, float %115
  %118 = add nuw nsw i64 %9, 68
  %119 = getelementptr inbounds float, float* %3, i64 %118
  %120 = load float, float* %119, align 4, !tbaa !193
  %121 = fcmp ogt float %117, %120
  %122 = select i1 %121, float %117, float %120
  %123 = add nuw nsw i64 %9, 69
  %124 = getelementptr inbounds float, float* %3, i64 %123
  %125 = load float, float* %124, align 4, !tbaa !193
  %126 = fcmp ogt float %122, %125
  %127 = select i1 %126, float %122, float %125
  %128 = add nuw nsw i64 %9, 70
  %129 = getelementptr inbounds float, float* %3, i64 %128
  %130 = load float, float* %129, align 4, !tbaa !193
  %131 = fcmp ogt float %127, %130
  %132 = select i1 %131, float %127, float %130
  store float %132, float* %87, align 4, !tbaa !196
  %133 = add nuw nsw i64 %7, 9
  %134 = getelementptr inbounds float, float* %2, i64 %133
  %135 = add nuw nsw i64 %9, 18
  %136 = getelementptr inbounds float, float* %3, i64 %135
  %137 = load float, float* %136, align 4, !tbaa !193
  %138 = fcmp olt float %137, 0xC7EFFFFFE0000000
  %139 = select i1 %138, float 0xC7EFFFFFE0000000, float %137
  %140 = or i64 %135, 1
  %141 = getelementptr inbounds float, float* %3, i64 %140
  %142 = load float, float* %141, align 4, !tbaa !193
  %143 = fcmp ogt float %139, %142
  %144 = select i1 %143, float %139, float %142
  %145 = add nuw nsw i64 %9, 20
  %146 = getelementptr inbounds float, float* %3, i64 %145
  %147 = load float, float* %146, align 4, !tbaa !193
  %148 = fcmp ogt float %144, %147
  %149 = select i1 %148, float %144, float %147
  %150 = add nuw nsw i64 %9, 44
  %151 = getelementptr inbounds float, float* %3, i64 %150
  %152 = load float, float* %151, align 4, !tbaa !193
  %153 = fcmp ogt float %149, %152
  %154 = select i1 %153, float %149, float %152
  %155 = add nuw nsw i64 %9, 45
  %156 = getelementptr inbounds float, float* %3, i64 %155
  %157 = load float, float* %156, align 4, !tbaa !193
  %158 = fcmp ogt float %154, %157
  %159 = select i1 %158, float %154, float %157
  %160 = add nuw nsw i64 %9, 46
  %161 = getelementptr inbounds float, float* %3, i64 %160
  %162 = load float, float* %161, align 4, !tbaa !193
  %163 = fcmp ogt float %159, %162
  %164 = select i1 %163, float %159, float %162
  %165 = add nuw nsw i64 %9, 70
  %166 = getelementptr inbounds float, float* %3, i64 %165
  %167 = load float, float* %166, align 4, !tbaa !193
  %168 = fcmp ogt float %164, %167
  %169 = select i1 %168, float %164, float %167
  %170 = add nuw nsw i64 %9, 71
  %171 = getelementptr inbounds float, float* %3, i64 %170
  %172 = load float, float* %171, align 4, !tbaa !193
  %173 = fcmp ogt float %169, %172
  %174 = select i1 %173, float %169, float %172
  %175 = add nuw nsw i64 %9, 72
  %176 = getelementptr inbounds float, float* %3, i64 %175
  %177 = load float, float* %176, align 4, !tbaa !193
  %178 = fcmp ogt float %174, %177
  %179 = select i1 %178, float %174, float %177
  store float %179, float* %134, align 4, !tbaa !196
  %180 = add nuw nsw i64 %7, 10
  %181 = getelementptr inbounds float, float* %2, i64 %180
  %182 = add nuw nsw i64 %9, 20
  %183 = getelementptr inbounds float, float* %3, i64 %182
  %184 = load float, float* %183, align 4, !tbaa !193
  %185 = fcmp olt float %184, 0xC7EFFFFFE0000000
  %186 = select i1 %185, float 0xC7EFFFFFE0000000, float %184
  %187 = or i64 %182, 1
  %188 = getelementptr inbounds float, float* %3, i64 %187
  %189 = load float, float* %188, align 4, !tbaa !193
  %190 = fcmp ogt float %186, %189
  %191 = select i1 %190, float %186, float %189
  %192 = add nuw nsw i64 %9, 22
  %193 = getelementptr inbounds float, float* %3, i64 %192
  %194 = load float, float* %193, align 4, !tbaa !193
  %195 = fcmp ogt float %191, %194
  %196 = select i1 %195, float %191, float %194
  %197 = add nuw nsw i64 %9, 46
  %198 = getelementptr inbounds float, float* %3, i64 %197
  %199 = load float, float* %198, align 4, !tbaa !193
  %200 = fcmp ogt float %196, %199
  %201 = select i1 %200, float %196, float %199
  %202 = add nuw nsw i64 %9, 47
  %203 = getelementptr inbounds float, float* %3, i64 %202
  %204 = load float, float* %203, align 4, !tbaa !193
  %205 = fcmp ogt float %201, %204
  %206 = select i1 %205, float %201, float %204
  %207 = add nuw nsw i64 %9, 48
  %208 = getelementptr inbounds float, float* %3, i64 %207
  %209 = load float, float* %208, align 4, !tbaa !193
  %210 = fcmp ogt float %206, %209
  %211 = select i1 %210, float %206, float %209
  %212 = add nuw nsw i64 %9, 72
  %213 = getelementptr inbounds float, float* %3, i64 %212
  %214 = load float, float* %213, align 4, !tbaa !193
  %215 = fcmp ogt float %211, %214
  %216 = select i1 %215, float %211, float %214
  %217 = add nuw nsw i64 %9, 73
  %218 = getelementptr inbounds float, float* %3, i64 %217
  %219 = load float, float* %218, align 4, !tbaa !193
  %220 = fcmp ogt float %216, %219
  %221 = select i1 %220, float %216, float %219
  %222 = add nuw nsw i64 %9, 74
  %223 = getelementptr inbounds float, float* %3, i64 %222
  %224 = load float, float* %223, align 4, !tbaa !193
  %225 = fcmp ogt float %221, %224
  %226 = select i1 %225, float %221, float %224
  store float %226, float* %181, align 4, !tbaa !196
  %227 = add nuw nsw i64 %7, 11
  %228 = getelementptr inbounds float, float* %2, i64 %227
  %229 = add nuw nsw i64 %9, 22
  %230 = getelementptr inbounds float, float* %3, i64 %229
  %231 = load float, float* %230, align 4, !tbaa !193
  %232 = fcmp olt float %231, 0xC7EFFFFFE0000000
  %233 = select i1 %232, float 0xC7EFFFFFE0000000, float %231
  %234 = or i64 %229, 1
  %235 = getelementptr inbounds float, float* %3, i64 %234
  %236 = load float, float* %235, align 4, !tbaa !193
  %237 = fcmp ogt float %233, %236
  %238 = select i1 %237, float %233, float %236
  %239 = add nuw nsw i64 %9, 24
  %240 = getelementptr inbounds float, float* %3, i64 %239
  %241 = load float, float* %240, align 4, !tbaa !193
  %242 = fcmp ogt float %238, %241
  %243 = select i1 %242, float %238, float %241
  %244 = add nuw nsw i64 %9, 48
  %245 = getelementptr inbounds float, float* %3, i64 %244
  %246 = load float, float* %245, align 4, !tbaa !193
  %247 = fcmp ogt float %243, %246
  %248 = select i1 %247, float %243, float %246
  %249 = add nuw nsw i64 %9, 49
  %250 = getelementptr inbounds float, float* %3, i64 %249
  %251 = load float, float* %250, align 4, !tbaa !193
  %252 = fcmp ogt float %248, %251
  %253 = select i1 %252, float %248, float %251
  %254 = add nuw nsw i64 %9, 50
  %255 = getelementptr inbounds float, float* %3, i64 %254
  %256 = load float, float* %255, align 4, !tbaa !193
  %257 = fcmp ogt float %253, %256
  %258 = select i1 %257, float %253, float %256
  %259 = add nuw nsw i64 %9, 74
  %260 = getelementptr inbounds float, float* %3, i64 %259
  %261 = load float, float* %260, align 4, !tbaa !193
  %262 = fcmp ogt float %258, %261
  %263 = select i1 %262, float %258, float %261
  %264 = add nuw nsw i64 %9, 75
  %265 = getelementptr inbounds float, float* %3, i64 %264
  %266 = load float, float* %265, align 4, !tbaa !193
  %267 = fcmp ogt float %263, %266
  %268 = select i1 %267, float %263, float %266
  %269 = add nuw nsw i64 %9, 76
  %270 = getelementptr inbounds float, float* %3, i64 %269
  %271 = load float, float* %270, align 4, !tbaa !193
  %272 = fcmp ogt float %268, %271
  %273 = select i1 %272, float %268, float %271
  store float %273, float* %228, align 4, !tbaa !196
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 12
  br i1 %exitcond5, label %for_end3, label %for_begin4.preheader, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  %exitcond8 = icmp eq i64 %indvars.iv.next7, 256
  br i1 %exitcond8, label %for_end, label %for_begin1.preheader, !prof !28
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
  %wide.load = load <4 x float>, <4 x float>* %7, align 4, !tbaa !199
  %8 = getelementptr inbounds float, float* %6, i64 4
  %9 = bitcast float* %8 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !199
  %10 = getelementptr inbounds float, float* %4, i64 %index
  %11 = bitcast float* %10 to <4 x float>*
  %wide.load3 = load <4 x float>, <4 x float>* %11, align 4, !tbaa !202
  %12 = getelementptr inbounds float, float* %10, i64 4
  %13 = bitcast float* %12 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %13, align 4, !tbaa !202
  %14 = fadd <4 x float> %wide.load, %wide.load3
  %15 = fadd <4 x float> %wide.load2, %wide.load4
  %16 = getelementptr inbounds float, float* %5, i64 %index
  %17 = bitcast float* %16 to <4 x float>*
  store <4 x float> %14, <4 x float>* %17, align 4, !tbaa !205
  %18 = getelementptr inbounds float, float* %16, i64 4
  %19 = bitcast float* %18 to <4 x float>*
  store <4 x float> %15, <4 x float>* %19, align 4, !tbaa !205
  %index.next = add i64 %index, 8
  %20 = icmp eq i64 %index.next, 1000
  br i1 %20, label %for_end, label %vector.body, !llvm.loop !208

for_end:                                          ; preds = %vector.body
  ret void
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
  %5 = load float, float* %4, align 64, !tbaa !209
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
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !223
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !223
  %11 = fmul <4 x float> %wide.load, %broadcast.splat4
  %12 = fmul <4 x float> %wide.load2, %broadcast.splat6
  %13 = getelementptr inbounds float, float* %6, i64 %index
  %14 = bitcast float* %13 to <4 x float>*
  store <4 x float> %11, <4 x float>* %14, align 4, !tbaa !226
  %15 = getelementptr inbounds float, float* %13, i64 4
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %12, <4 x float>* %16, align 4, !tbaa !226
  %index.next = add i64 %index, 8
  %17 = icmp eq i64 %index.next, 4096
  br i1 %17, label %for_end, label %vector.body, !llvm.loop !229

for_end:                                          ; preds = %vector.body
  ret void
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
  %5 = load float, float* %4, align 64, !tbaa !230
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
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !244
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !244
  %11 = fmul <4 x float> %wide.load, %broadcast.splat4
  %12 = fmul <4 x float> %wide.load2, %broadcast.splat6
  %13 = getelementptr inbounds float, float* %6, i64 %index
  %14 = bitcast float* %13 to <4 x float>*
  store <4 x float> %11, <4 x float>* %14, align 4, !tbaa !247
  %15 = getelementptr inbounds float, float* %13, i64 4
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %12, <4 x float>* %16, align 4, !tbaa !247
  %index.next = add i64 %index, 8
  %17 = icmp eq i64 %index.next, 4096
  br i1 %17, label %for_end, label %vector.body, !llvm.loop !250

for_end:                                          ; preds = %vector.body
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

for_begin1.preheader:                             ; preds = %for_begin1.preheader, %entry
  %indvars.iv1 = phi i64 [ 0, %entry ], [ %indvars.iv.next2, %for_begin1.preheader ]
  %4 = mul nuw nsw i64 %indvars.iv1, 144
  %5 = getelementptr inbounds float, float* %2, i64 %4
  %6 = getelementptr inbounds float, float* %3, i64 %4
  %7 = bitcast float* %5 to <4 x float>*
  %8 = load <4 x float>, <4 x float>* %7, align 4, !tbaa !251
  %9 = fcmp ogt <4 x float> %8, zeroinitializer
  %10 = select <4 x i1> %9, <4 x float> %8, <4 x float> zeroinitializer
  %11 = bitcast float* %6 to <4 x float>*
  store <4 x float> %10, <4 x float>* %11, align 4, !tbaa !254
  %12 = or i64 %4, 4
  %13 = getelementptr inbounds float, float* %2, i64 %12
  %14 = getelementptr inbounds float, float* %3, i64 %12
  %15 = bitcast float* %13 to <4 x float>*
  %16 = load <4 x float>, <4 x float>* %15, align 4, !tbaa !251
  %17 = fcmp ogt <4 x float> %16, zeroinitializer
  %18 = select <4 x i1> %17, <4 x float> %16, <4 x float> zeroinitializer
  %19 = bitcast float* %14 to <4 x float>*
  store <4 x float> %18, <4 x float>* %19, align 4, !tbaa !254
  %20 = or i64 %4, 8
  %21 = getelementptr inbounds float, float* %2, i64 %20
  %22 = getelementptr inbounds float, float* %3, i64 %20
  %23 = bitcast float* %21 to <4 x float>*
  %24 = load <4 x float>, <4 x float>* %23, align 4, !tbaa !251
  %25 = fcmp ogt <4 x float> %24, zeroinitializer
  %26 = select <4 x i1> %25, <4 x float> %24, <4 x float> zeroinitializer
  %27 = bitcast float* %22 to <4 x float>*
  store <4 x float> %26, <4 x float>* %27, align 4, !tbaa !254
  %28 = or i64 %4, 12
  %29 = getelementptr inbounds float, float* %2, i64 %28
  %30 = getelementptr inbounds float, float* %3, i64 %28
  %31 = bitcast float* %29 to <4 x float>*
  %32 = load <4 x float>, <4 x float>* %31, align 4, !tbaa !251
  %33 = fcmp ogt <4 x float> %32, zeroinitializer
  %34 = select <4 x i1> %33, <4 x float> %32, <4 x float> zeroinitializer
  %35 = bitcast float* %30 to <4 x float>*
  store <4 x float> %34, <4 x float>* %35, align 4, !tbaa !254
  %36 = add nuw nsw i64 %28, 4
  %37 = getelementptr inbounds float, float* %2, i64 %36
  %38 = getelementptr inbounds float, float* %3, i64 %36
  %39 = bitcast float* %37 to <4 x float>*
  %40 = load <4 x float>, <4 x float>* %39, align 4, !tbaa !251
  %41 = fcmp ogt <4 x float> %40, zeroinitializer
  %42 = select <4 x i1> %41, <4 x float> %40, <4 x float> zeroinitializer
  %43 = bitcast float* %38 to <4 x float>*
  store <4 x float> %42, <4 x float>* %43, align 4, !tbaa !254
  %44 = add nuw nsw i64 %28, 8
  %45 = getelementptr inbounds float, float* %2, i64 %44
  %46 = getelementptr inbounds float, float* %3, i64 %44
  %47 = bitcast float* %45 to <4 x float>*
  %48 = load <4 x float>, <4 x float>* %47, align 4, !tbaa !251
  %49 = fcmp ogt <4 x float> %48, zeroinitializer
  %50 = select <4 x i1> %49, <4 x float> %48, <4 x float> zeroinitializer
  %51 = bitcast float* %46 to <4 x float>*
  store <4 x float> %50, <4 x float>* %51, align 4, !tbaa !254
  %52 = add nuw nsw i64 %4, 24
  %53 = getelementptr inbounds float, float* %2, i64 %52
  %54 = getelementptr inbounds float, float* %3, i64 %52
  %55 = bitcast float* %53 to <4 x float>*
  %56 = load <4 x float>, <4 x float>* %55, align 4, !tbaa !251
  %57 = fcmp ogt <4 x float> %56, zeroinitializer
  %58 = select <4 x i1> %57, <4 x float> %56, <4 x float> zeroinitializer
  %59 = bitcast float* %54 to <4 x float>*
  store <4 x float> %58, <4 x float>* %59, align 4, !tbaa !254
  %60 = add nuw nsw i64 %4, 28
  %61 = getelementptr inbounds float, float* %2, i64 %60
  %62 = getelementptr inbounds float, float* %3, i64 %60
  %63 = bitcast float* %61 to <4 x float>*
  %64 = load <4 x float>, <4 x float>* %63, align 4, !tbaa !251
  %65 = fcmp ogt <4 x float> %64, zeroinitializer
  %66 = select <4 x i1> %65, <4 x float> %64, <4 x float> zeroinitializer
  %67 = bitcast float* %62 to <4 x float>*
  store <4 x float> %66, <4 x float>* %67, align 4, !tbaa !254
  %68 = add nuw nsw i64 %4, 32
  %69 = getelementptr inbounds float, float* %2, i64 %68
  %70 = getelementptr inbounds float, float* %3, i64 %68
  %71 = bitcast float* %69 to <4 x float>*
  %72 = load <4 x float>, <4 x float>* %71, align 4, !tbaa !251
  %73 = fcmp ogt <4 x float> %72, zeroinitializer
  %74 = select <4 x i1> %73, <4 x float> %72, <4 x float> zeroinitializer
  %75 = bitcast float* %70 to <4 x float>*
  store <4 x float> %74, <4 x float>* %75, align 4, !tbaa !254
  %76 = add nuw nsw i64 %4, 36
  %77 = getelementptr inbounds float, float* %2, i64 %76
  %78 = getelementptr inbounds float, float* %3, i64 %76
  %79 = bitcast float* %77 to <4 x float>*
  %80 = load <4 x float>, <4 x float>* %79, align 4, !tbaa !251
  %81 = fcmp ogt <4 x float> %80, zeroinitializer
  %82 = select <4 x i1> %81, <4 x float> %80, <4 x float> zeroinitializer
  %83 = bitcast float* %78 to <4 x float>*
  store <4 x float> %82, <4 x float>* %83, align 4, !tbaa !254
  %84 = add nuw nsw i64 %4, 40
  %85 = getelementptr inbounds float, float* %2, i64 %84
  %86 = getelementptr inbounds float, float* %3, i64 %84
  %87 = bitcast float* %85 to <4 x float>*
  %88 = load <4 x float>, <4 x float>* %87, align 4, !tbaa !251
  %89 = fcmp ogt <4 x float> %88, zeroinitializer
  %90 = select <4 x i1> %89, <4 x float> %88, <4 x float> zeroinitializer
  %91 = bitcast float* %86 to <4 x float>*
  store <4 x float> %90, <4 x float>* %91, align 4, !tbaa !254
  %92 = add nuw nsw i64 %4, 44
  %93 = getelementptr inbounds float, float* %2, i64 %92
  %94 = getelementptr inbounds float, float* %3, i64 %92
  %95 = bitcast float* %93 to <4 x float>*
  %96 = load <4 x float>, <4 x float>* %95, align 4, !tbaa !251
  %97 = fcmp ogt <4 x float> %96, zeroinitializer
  %98 = select <4 x i1> %97, <4 x float> %96, <4 x float> zeroinitializer
  %99 = bitcast float* %94 to <4 x float>*
  store <4 x float> %98, <4 x float>* %99, align 4, !tbaa !254
  %100 = add nuw nsw i64 %4, 48
  %101 = getelementptr inbounds float, float* %2, i64 %100
  %102 = getelementptr inbounds float, float* %3, i64 %100
  %103 = bitcast float* %101 to <4 x float>*
  %104 = load <4 x float>, <4 x float>* %103, align 4, !tbaa !251
  %105 = fcmp ogt <4 x float> %104, zeroinitializer
  %106 = select <4 x i1> %105, <4 x float> %104, <4 x float> zeroinitializer
  %107 = bitcast float* %102 to <4 x float>*
  store <4 x float> %106, <4 x float>* %107, align 4, !tbaa !254
  %108 = add nuw nsw i64 %4, 52
  %109 = getelementptr inbounds float, float* %2, i64 %108
  %110 = getelementptr inbounds float, float* %3, i64 %108
  %111 = bitcast float* %109 to <4 x float>*
  %112 = load <4 x float>, <4 x float>* %111, align 4, !tbaa !251
  %113 = fcmp ogt <4 x float> %112, zeroinitializer
  %114 = select <4 x i1> %113, <4 x float> %112, <4 x float> zeroinitializer
  %115 = bitcast float* %110 to <4 x float>*
  store <4 x float> %114, <4 x float>* %115, align 4, !tbaa !254
  %116 = add nuw nsw i64 %4, 56
  %117 = getelementptr inbounds float, float* %2, i64 %116
  %118 = getelementptr inbounds float, float* %3, i64 %116
  %119 = bitcast float* %117 to <4 x float>*
  %120 = load <4 x float>, <4 x float>* %119, align 4, !tbaa !251
  %121 = fcmp ogt <4 x float> %120, zeroinitializer
  %122 = select <4 x i1> %121, <4 x float> %120, <4 x float> zeroinitializer
  %123 = bitcast float* %118 to <4 x float>*
  store <4 x float> %122, <4 x float>* %123, align 4, !tbaa !254
  %124 = add nuw nsw i64 %4, 60
  %125 = getelementptr inbounds float, float* %2, i64 %124
  %126 = getelementptr inbounds float, float* %3, i64 %124
  %127 = bitcast float* %125 to <4 x float>*
  %128 = load <4 x float>, <4 x float>* %127, align 4, !tbaa !251
  %129 = fcmp ogt <4 x float> %128, zeroinitializer
  %130 = select <4 x i1> %129, <4 x float> %128, <4 x float> zeroinitializer
  %131 = bitcast float* %126 to <4 x float>*
  store <4 x float> %130, <4 x float>* %131, align 4, !tbaa !254
  %132 = add nuw nsw i64 %4, 64
  %133 = getelementptr inbounds float, float* %2, i64 %132
  %134 = getelementptr inbounds float, float* %3, i64 %132
  %135 = bitcast float* %133 to <4 x float>*
  %136 = load <4 x float>, <4 x float>* %135, align 4, !tbaa !251
  %137 = fcmp ogt <4 x float> %136, zeroinitializer
  %138 = select <4 x i1> %137, <4 x float> %136, <4 x float> zeroinitializer
  %139 = bitcast float* %134 to <4 x float>*
  store <4 x float> %138, <4 x float>* %139, align 4, !tbaa !254
  %140 = add nuw nsw i64 %4, 68
  %141 = getelementptr inbounds float, float* %2, i64 %140
  %142 = getelementptr inbounds float, float* %3, i64 %140
  %143 = bitcast float* %141 to <4 x float>*
  %144 = load <4 x float>, <4 x float>* %143, align 4, !tbaa !251
  %145 = fcmp ogt <4 x float> %144, zeroinitializer
  %146 = select <4 x i1> %145, <4 x float> %144, <4 x float> zeroinitializer
  %147 = bitcast float* %142 to <4 x float>*
  store <4 x float> %146, <4 x float>* %147, align 4, !tbaa !254
  %148 = add nuw nsw i64 %4, 72
  %149 = getelementptr inbounds float, float* %2, i64 %148
  %150 = getelementptr inbounds float, float* %3, i64 %148
  %151 = bitcast float* %149 to <4 x float>*
  %152 = load <4 x float>, <4 x float>* %151, align 4, !tbaa !251
  %153 = fcmp ogt <4 x float> %152, zeroinitializer
  %154 = select <4 x i1> %153, <4 x float> %152, <4 x float> zeroinitializer
  %155 = bitcast float* %150 to <4 x float>*
  store <4 x float> %154, <4 x float>* %155, align 4, !tbaa !254
  %156 = add nuw nsw i64 %4, 76
  %157 = getelementptr inbounds float, float* %2, i64 %156
  %158 = getelementptr inbounds float, float* %3, i64 %156
  %159 = bitcast float* %157 to <4 x float>*
  %160 = load <4 x float>, <4 x float>* %159, align 4, !tbaa !251
  %161 = fcmp ogt <4 x float> %160, zeroinitializer
  %162 = select <4 x i1> %161, <4 x float> %160, <4 x float> zeroinitializer
  %163 = bitcast float* %158 to <4 x float>*
  store <4 x float> %162, <4 x float>* %163, align 4, !tbaa !254
  %164 = add nuw nsw i64 %4, 80
  %165 = getelementptr inbounds float, float* %2, i64 %164
  %166 = getelementptr inbounds float, float* %3, i64 %164
  %167 = bitcast float* %165 to <4 x float>*
  %168 = load <4 x float>, <4 x float>* %167, align 4, !tbaa !251
  %169 = fcmp ogt <4 x float> %168, zeroinitializer
  %170 = select <4 x i1> %169, <4 x float> %168, <4 x float> zeroinitializer
  %171 = bitcast float* %166 to <4 x float>*
  store <4 x float> %170, <4 x float>* %171, align 4, !tbaa !254
  %172 = add nuw nsw i64 %4, 84
  %173 = getelementptr inbounds float, float* %2, i64 %172
  %174 = getelementptr inbounds float, float* %3, i64 %172
  %175 = bitcast float* %173 to <4 x float>*
  %176 = load <4 x float>, <4 x float>* %175, align 4, !tbaa !251
  %177 = fcmp ogt <4 x float> %176, zeroinitializer
  %178 = select <4 x i1> %177, <4 x float> %176, <4 x float> zeroinitializer
  %179 = bitcast float* %174 to <4 x float>*
  store <4 x float> %178, <4 x float>* %179, align 4, !tbaa !254
  %180 = add nuw nsw i64 %4, 88
  %181 = getelementptr inbounds float, float* %2, i64 %180
  %182 = getelementptr inbounds float, float* %3, i64 %180
  %183 = bitcast float* %181 to <4 x float>*
  %184 = load <4 x float>, <4 x float>* %183, align 4, !tbaa !251
  %185 = fcmp ogt <4 x float> %184, zeroinitializer
  %186 = select <4 x i1> %185, <4 x float> %184, <4 x float> zeroinitializer
  %187 = bitcast float* %182 to <4 x float>*
  store <4 x float> %186, <4 x float>* %187, align 4, !tbaa !254
  %188 = add nuw nsw i64 %4, 92
  %189 = getelementptr inbounds float, float* %2, i64 %188
  %190 = getelementptr inbounds float, float* %3, i64 %188
  %191 = bitcast float* %189 to <4 x float>*
  %192 = load <4 x float>, <4 x float>* %191, align 4, !tbaa !251
  %193 = fcmp ogt <4 x float> %192, zeroinitializer
  %194 = select <4 x i1> %193, <4 x float> %192, <4 x float> zeroinitializer
  %195 = bitcast float* %190 to <4 x float>*
  store <4 x float> %194, <4 x float>* %195, align 4, !tbaa !254
  %196 = add nuw nsw i64 %4, 96
  %197 = getelementptr inbounds float, float* %2, i64 %196
  %198 = getelementptr inbounds float, float* %3, i64 %196
  %199 = bitcast float* %197 to <4 x float>*
  %200 = load <4 x float>, <4 x float>* %199, align 4, !tbaa !251
  %201 = fcmp ogt <4 x float> %200, zeroinitializer
  %202 = select <4 x i1> %201, <4 x float> %200, <4 x float> zeroinitializer
  %203 = bitcast float* %198 to <4 x float>*
  store <4 x float> %202, <4 x float>* %203, align 4, !tbaa !254
  %204 = add nuw nsw i64 %4, 100
  %205 = getelementptr inbounds float, float* %2, i64 %204
  %206 = getelementptr inbounds float, float* %3, i64 %204
  %207 = bitcast float* %205 to <4 x float>*
  %208 = load <4 x float>, <4 x float>* %207, align 4, !tbaa !251
  %209 = fcmp ogt <4 x float> %208, zeroinitializer
  %210 = select <4 x i1> %209, <4 x float> %208, <4 x float> zeroinitializer
  %211 = bitcast float* %206 to <4 x float>*
  store <4 x float> %210, <4 x float>* %211, align 4, !tbaa !254
  %212 = add nuw nsw i64 %4, 104
  %213 = getelementptr inbounds float, float* %2, i64 %212
  %214 = getelementptr inbounds float, float* %3, i64 %212
  %215 = bitcast float* %213 to <4 x float>*
  %216 = load <4 x float>, <4 x float>* %215, align 4, !tbaa !251
  %217 = fcmp ogt <4 x float> %216, zeroinitializer
  %218 = select <4 x i1> %217, <4 x float> %216, <4 x float> zeroinitializer
  %219 = bitcast float* %214 to <4 x float>*
  store <4 x float> %218, <4 x float>* %219, align 4, !tbaa !254
  %220 = add nuw nsw i64 %4, 108
  %221 = getelementptr inbounds float, float* %2, i64 %220
  %222 = getelementptr inbounds float, float* %3, i64 %220
  %223 = bitcast float* %221 to <4 x float>*
  %224 = load <4 x float>, <4 x float>* %223, align 4, !tbaa !251
  %225 = fcmp ogt <4 x float> %224, zeroinitializer
  %226 = select <4 x i1> %225, <4 x float> %224, <4 x float> zeroinitializer
  %227 = bitcast float* %222 to <4 x float>*
  store <4 x float> %226, <4 x float>* %227, align 4, !tbaa !254
  %228 = add nuw nsw i64 %4, 112
  %229 = getelementptr inbounds float, float* %2, i64 %228
  %230 = getelementptr inbounds float, float* %3, i64 %228
  %231 = bitcast float* %229 to <4 x float>*
  %232 = load <4 x float>, <4 x float>* %231, align 4, !tbaa !251
  %233 = fcmp ogt <4 x float> %232, zeroinitializer
  %234 = select <4 x i1> %233, <4 x float> %232, <4 x float> zeroinitializer
  %235 = bitcast float* %230 to <4 x float>*
  store <4 x float> %234, <4 x float>* %235, align 4, !tbaa !254
  %236 = add nuw nsw i64 %4, 116
  %237 = getelementptr inbounds float, float* %2, i64 %236
  %238 = getelementptr inbounds float, float* %3, i64 %236
  %239 = bitcast float* %237 to <4 x float>*
  %240 = load <4 x float>, <4 x float>* %239, align 4, !tbaa !251
  %241 = fcmp ogt <4 x float> %240, zeroinitializer
  %242 = select <4 x i1> %241, <4 x float> %240, <4 x float> zeroinitializer
  %243 = bitcast float* %238 to <4 x float>*
  store <4 x float> %242, <4 x float>* %243, align 4, !tbaa !254
  %244 = add nuw nsw i64 %4, 120
  %245 = getelementptr inbounds float, float* %2, i64 %244
  %246 = getelementptr inbounds float, float* %3, i64 %244
  %247 = bitcast float* %245 to <4 x float>*
  %248 = load <4 x float>, <4 x float>* %247, align 4, !tbaa !251
  %249 = fcmp ogt <4 x float> %248, zeroinitializer
  %250 = select <4 x i1> %249, <4 x float> %248, <4 x float> zeroinitializer
  %251 = bitcast float* %246 to <4 x float>*
  store <4 x float> %250, <4 x float>* %251, align 4, !tbaa !254
  %252 = add nuw nsw i64 %4, 124
  %253 = getelementptr inbounds float, float* %2, i64 %252
  %254 = getelementptr inbounds float, float* %3, i64 %252
  %255 = bitcast float* %253 to <4 x float>*
  %256 = load <4 x float>, <4 x float>* %255, align 4, !tbaa !251
  %257 = fcmp ogt <4 x float> %256, zeroinitializer
  %258 = select <4 x i1> %257, <4 x float> %256, <4 x float> zeroinitializer
  %259 = bitcast float* %254 to <4 x float>*
  store <4 x float> %258, <4 x float>* %259, align 4, !tbaa !254
  %260 = add nuw nsw i64 %4, 128
  %261 = getelementptr inbounds float, float* %2, i64 %260
  %262 = getelementptr inbounds float, float* %3, i64 %260
  %263 = bitcast float* %261 to <4 x float>*
  %264 = load <4 x float>, <4 x float>* %263, align 4, !tbaa !251
  %265 = fcmp ogt <4 x float> %264, zeroinitializer
  %266 = select <4 x i1> %265, <4 x float> %264, <4 x float> zeroinitializer
  %267 = bitcast float* %262 to <4 x float>*
  store <4 x float> %266, <4 x float>* %267, align 4, !tbaa !254
  %268 = add nuw nsw i64 %4, 132
  %269 = getelementptr inbounds float, float* %2, i64 %268
  %270 = getelementptr inbounds float, float* %3, i64 %268
  %271 = bitcast float* %269 to <4 x float>*
  %272 = load <4 x float>, <4 x float>* %271, align 4, !tbaa !251
  %273 = fcmp ogt <4 x float> %272, zeroinitializer
  %274 = select <4 x i1> %273, <4 x float> %272, <4 x float> zeroinitializer
  %275 = bitcast float* %270 to <4 x float>*
  store <4 x float> %274, <4 x float>* %275, align 4, !tbaa !254
  %276 = add nuw nsw i64 %4, 136
  %277 = getelementptr inbounds float, float* %2, i64 %276
  %278 = getelementptr inbounds float, float* %3, i64 %276
  %279 = bitcast float* %277 to <4 x float>*
  %280 = load <4 x float>, <4 x float>* %279, align 4, !tbaa !251
  %281 = fcmp ogt <4 x float> %280, zeroinitializer
  %282 = select <4 x i1> %281, <4 x float> %280, <4 x float> zeroinitializer
  %283 = bitcast float* %278 to <4 x float>*
  store <4 x float> %282, <4 x float>* %283, align 4, !tbaa !254
  %284 = add nuw nsw i64 %4, 140
  %285 = getelementptr inbounds float, float* %2, i64 %284
  %286 = getelementptr inbounds float, float* %3, i64 %284
  %287 = bitcast float* %285 to <4 x float>*
  %288 = load <4 x float>, <4 x float>* %287, align 4, !tbaa !251
  %289 = fcmp ogt <4 x float> %288, zeroinitializer
  %290 = select <4 x i1> %289, <4 x float> %288, <4 x float> zeroinitializer
  %291 = bitcast float* %286 to <4 x float>*
  store <4 x float> %290, <4 x float>* %291, align 4, !tbaa !254
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 256
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !28

for_end:                                          ; preds = %for_begin1.preheader
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
  %wide.load = load <4 x float>, <4 x float>* %7, align 4, !tbaa !257
  %8 = getelementptr inbounds float, float* %6, i64 4
  %9 = bitcast float* %8 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !257
  %10 = getelementptr inbounds float, float* %4, i64 %index
  %11 = bitcast float* %10 to <4 x float>*
  %wide.load3 = load <4 x float>, <4 x float>* %11, align 4, !tbaa !260
  %12 = getelementptr inbounds float, float* %10, i64 4
  %13 = bitcast float* %12 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %13, align 4, !tbaa !260
  %14 = fadd <4 x float> %wide.load, %wide.load3
  %15 = fadd <4 x float> %wide.load2, %wide.load4
  %16 = getelementptr inbounds float, float* %5, i64 %index
  %17 = bitcast float* %16 to <4 x float>*
  store <4 x float> %14, <4 x float>* %17, align 4, !tbaa !263
  %18 = getelementptr inbounds float, float* %16, i64 4
  %19 = bitcast float* %18 to <4 x float>*
  store <4 x float> %15, <4 x float>* %19, align 4, !tbaa !263
  %index.next = add i64 %index, 8
  %20 = icmp eq i64 %index.next, 4096
  br i1 %20, label %for_end, label %vector.body, !llvm.loop !266

for_end:                                          ; preds = %vector.body
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
  %5 = load float, float* %4, align 64, !tbaa !267
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
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !281
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !281
  %11 = fmul <4 x float> %wide.load, %broadcast.splat4
  %12 = fmul <4 x float> %wide.load2, %broadcast.splat6
  %13 = getelementptr inbounds float, float* %6, i64 %index
  %14 = bitcast float* %13 to <4 x float>*
  store <4 x float> %11, <4 x float>* %14, align 4, !tbaa !284
  %15 = getelementptr inbounds float, float* %13, i64 4
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %12, <4 x float>* %16, align 4, !tbaa !284
  %index.next = add i64 %index, 8
  %17 = icmp eq i64 %index.next, 9216
  br i1 %17, label %for_end, label %vector.body, !llvm.loop !287

for_end:                                          ; preds = %vector.body
  ret void
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
  %6 = mul nuw nsw i64 %indvars.iv3, 9216
  br label %for_body2

for_end:                                          ; preds = %for_end3
  ret void

for_body2:                                        ; preds = %for_body2, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_body2 ]
  %.02 = phi <16 x float> [ zeroinitializer, %for_begin1.preheader ], [ %15, %for_body2 ]
  %7 = shl nsw i64 %indvars.iv, 4
  %8 = getelementptr inbounds float, float* %3, i64 %7
  %9 = bitcast float* %8 to <16 x float>*
  %10 = load <16 x float>, <16 x float>* %9, align 64, !tbaa !288
  %11 = add nuw nsw i64 %7, %6
  %12 = getelementptr inbounds float, float* %4, i64 %11
  %13 = bitcast float* %12 to <16 x float>*
  %14 = load <16 x float>, <16 x float>* %13, align 64, !tbaa !291
  %15 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %10, <16 x float> %14, <16 x float> %.02)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 576
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
  store float %32, float* %16, align 4, !tbaa !294
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 4096
  br i1 %exitcond5, label %for_end, label %for_begin1.preheader, !prof !28
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
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 36864, i1 false)
  ret void
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
  %2 = bitcast i8* %1 to float*
  %3 = bitcast i8* %0 to float*
  br label %for_body

for_body:                                         ; preds = %for_body, %entry
  %indvars.iv = phi i64 [ 0, %entry ], [ %indvars.iv.next, %for_body ]
  %4 = phi i32 [ 0, %entry ], [ %12, %for_body ]
  %.lhs.trunc = trunc i32 %4 to i16
  %5 = urem i16 %.lhs.trunc, 6400
  %6 = zext i16 %5 to i64
  %7 = getelementptr inbounds float, float* %2, i64 %6
  %8 = bitcast float* %7 to i32*
  %9 = load i32, i32* %8, align 4, !tbaa !297
  %10 = getelementptr inbounds float, float* %3, i64 %indvars.iv
  %11 = bitcast float* %10 to i32*
  store i32 %9, i32* %11, align 4, !tbaa !300
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %12 = add nuw nsw i32 %4, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 9216
  br i1 %exitcond, label %for_end, label %for_body, !prof !28

for_end:                                          ; preds = %for_body
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
  %wide.load = load <4 x float>, <4 x float>* %5, align 4, !tbaa !303
  %6 = getelementptr inbounds float, float* %4, i64 4
  %7 = bitcast float* %6 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %7, align 4, !tbaa !303
  %8 = fcmp ogt <4 x float> %wide.load, zeroinitializer
  %9 = fcmp ogt <4 x float> %wide.load2, zeroinitializer
  %10 = select <4 x i1> %8, <4 x float> %wide.load, <4 x float> zeroinitializer
  %11 = select <4 x i1> %9, <4 x float> %wide.load2, <4 x float> zeroinitializer
  %12 = getelementptr inbounds float, float* %3, i64 %index
  %13 = bitcast float* %12 to <4 x float>*
  store <4 x float> %10, <4 x float>* %13, align 4, !tbaa !306
  %14 = getelementptr inbounds float, float* %12, i64 4
  %15 = bitcast float* %14 to <4 x float>*
  store <4 x float> %11, <4 x float>* %15, align 4, !tbaa !306
  %index.next = add i64 %index, 8
  %16 = icmp eq i64 %index.next, 4096
  br i1 %16, label %for_end, label %vector.body, !llvm.loop !309

for_end:                                          ; preds = %vector.body
  ret void
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
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_begin1.preheader, %entry
  %indvars.iv1 = phi i64 [ 0, %entry ], [ %indvars.iv.next2, %for_begin1.preheader ]
  %6 = mul nuw nsw i64 %indvars.iv1, 144
  %7 = getelementptr inbounds float, float* %3, i64 %indvars.iv1
  %8 = load float, float* %7, align 4, !tbaa !310
  %9 = insertelement <4 x float> undef, float %8, i32 0
  %10 = shufflevector <4 x float> %9, <4 x float> undef, <4 x i32> zeroinitializer
  %11 = insertelement <4 x float> undef, float %8, i32 0
  %12 = shufflevector <4 x float> %11, <4 x float> undef, <4 x i32> zeroinitializer
  %13 = insertelement <4 x float> undef, float %8, i32 0
  %14 = shufflevector <4 x float> %13, <4 x float> undef, <4 x i32> zeroinitializer
  %15 = getelementptr inbounds float, float* %4, i64 %6
  %16 = getelementptr inbounds float, float* %5, i64 %6
  %17 = bitcast float* %15 to <4 x float>*
  %18 = load <4 x float>, <4 x float>* %17, align 4, !tbaa !313
  %19 = fadd <4 x float> %10, %18
  %20 = bitcast float* %16 to <4 x float>*
  store <4 x float> %19, <4 x float>* %20, align 4, !tbaa !316
  %21 = or i64 %6, 4
  %22 = getelementptr inbounds float, float* %4, i64 %21
  %23 = getelementptr inbounds float, float* %5, i64 %21
  %24 = bitcast float* %22 to <4 x float>*
  %25 = load <4 x float>, <4 x float>* %24, align 4, !tbaa !313
  %26 = fadd <4 x float> %12, %25
  %27 = bitcast float* %23 to <4 x float>*
  store <4 x float> %26, <4 x float>* %27, align 4, !tbaa !316
  %28 = or i64 %6, 8
  %29 = getelementptr inbounds float, float* %4, i64 %28
  %30 = getelementptr inbounds float, float* %5, i64 %28
  %31 = bitcast float* %29 to <4 x float>*
  %32 = load <4 x float>, <4 x float>* %31, align 4, !tbaa !313
  %33 = fadd <4 x float> %14, %32
  %34 = bitcast float* %30 to <4 x float>*
  store <4 x float> %33, <4 x float>* %34, align 4, !tbaa !316
  %35 = or i64 %6, 12
  %36 = getelementptr inbounds float, float* %4, i64 %35
  %37 = getelementptr inbounds float, float* %5, i64 %35
  %38 = bitcast float* %36 to <4 x float>*
  %39 = load <4 x float>, <4 x float>* %38, align 4, !tbaa !313
  %40 = fadd <4 x float> %10, %39
  %41 = bitcast float* %37 to <4 x float>*
  store <4 x float> %40, <4 x float>* %41, align 4, !tbaa !316
  %42 = add nuw nsw i64 %35, 4
  %43 = getelementptr inbounds float, float* %4, i64 %42
  %44 = getelementptr inbounds float, float* %5, i64 %42
  %45 = bitcast float* %43 to <4 x float>*
  %46 = load <4 x float>, <4 x float>* %45, align 4, !tbaa !313
  %47 = fadd <4 x float> %12, %46
  %48 = bitcast float* %44 to <4 x float>*
  store <4 x float> %47, <4 x float>* %48, align 4, !tbaa !316
  %49 = add nuw nsw i64 %35, 8
  %50 = getelementptr inbounds float, float* %4, i64 %49
  %51 = getelementptr inbounds float, float* %5, i64 %49
  %52 = bitcast float* %50 to <4 x float>*
  %53 = load <4 x float>, <4 x float>* %52, align 4, !tbaa !313
  %54 = fadd <4 x float> %14, %53
  %55 = bitcast float* %51 to <4 x float>*
  store <4 x float> %54, <4 x float>* %55, align 4, !tbaa !316
  %56 = add nuw nsw i64 %6, 24
  %57 = getelementptr inbounds float, float* %4, i64 %56
  %58 = getelementptr inbounds float, float* %5, i64 %56
  %59 = bitcast float* %57 to <4 x float>*
  %60 = load <4 x float>, <4 x float>* %59, align 4, !tbaa !313
  %61 = fadd <4 x float> %10, %60
  %62 = bitcast float* %58 to <4 x float>*
  store <4 x float> %61, <4 x float>* %62, align 4, !tbaa !316
  %63 = add nuw nsw i64 %6, 28
  %64 = getelementptr inbounds float, float* %4, i64 %63
  %65 = getelementptr inbounds float, float* %5, i64 %63
  %66 = bitcast float* %64 to <4 x float>*
  %67 = load <4 x float>, <4 x float>* %66, align 4, !tbaa !313
  %68 = fadd <4 x float> %12, %67
  %69 = bitcast float* %65 to <4 x float>*
  store <4 x float> %68, <4 x float>* %69, align 4, !tbaa !316
  %70 = add nuw nsw i64 %6, 32
  %71 = getelementptr inbounds float, float* %4, i64 %70
  %72 = getelementptr inbounds float, float* %5, i64 %70
  %73 = bitcast float* %71 to <4 x float>*
  %74 = load <4 x float>, <4 x float>* %73, align 4, !tbaa !313
  %75 = fadd <4 x float> %14, %74
  %76 = bitcast float* %72 to <4 x float>*
  store <4 x float> %75, <4 x float>* %76, align 4, !tbaa !316
  %77 = add nuw nsw i64 %6, 36
  %78 = getelementptr inbounds float, float* %4, i64 %77
  %79 = getelementptr inbounds float, float* %5, i64 %77
  %80 = bitcast float* %78 to <4 x float>*
  %81 = load <4 x float>, <4 x float>* %80, align 4, !tbaa !313
  %82 = fadd <4 x float> %10, %81
  %83 = bitcast float* %79 to <4 x float>*
  store <4 x float> %82, <4 x float>* %83, align 4, !tbaa !316
  %84 = add nuw nsw i64 %6, 40
  %85 = getelementptr inbounds float, float* %4, i64 %84
  %86 = getelementptr inbounds float, float* %5, i64 %84
  %87 = bitcast float* %85 to <4 x float>*
  %88 = load <4 x float>, <4 x float>* %87, align 4, !tbaa !313
  %89 = fadd <4 x float> %12, %88
  %90 = bitcast float* %86 to <4 x float>*
  store <4 x float> %89, <4 x float>* %90, align 4, !tbaa !316
  %91 = add nuw nsw i64 %6, 44
  %92 = getelementptr inbounds float, float* %4, i64 %91
  %93 = getelementptr inbounds float, float* %5, i64 %91
  %94 = bitcast float* %92 to <4 x float>*
  %95 = load <4 x float>, <4 x float>* %94, align 4, !tbaa !313
  %96 = fadd <4 x float> %14, %95
  %97 = bitcast float* %93 to <4 x float>*
  store <4 x float> %96, <4 x float>* %97, align 4, !tbaa !316
  %98 = add nuw nsw i64 %6, 48
  %99 = getelementptr inbounds float, float* %4, i64 %98
  %100 = getelementptr inbounds float, float* %5, i64 %98
  %101 = bitcast float* %99 to <4 x float>*
  %102 = load <4 x float>, <4 x float>* %101, align 4, !tbaa !313
  %103 = fadd <4 x float> %10, %102
  %104 = bitcast float* %100 to <4 x float>*
  store <4 x float> %103, <4 x float>* %104, align 4, !tbaa !316
  %105 = add nuw nsw i64 %6, 52
  %106 = getelementptr inbounds float, float* %4, i64 %105
  %107 = getelementptr inbounds float, float* %5, i64 %105
  %108 = bitcast float* %106 to <4 x float>*
  %109 = load <4 x float>, <4 x float>* %108, align 4, !tbaa !313
  %110 = fadd <4 x float> %12, %109
  %111 = bitcast float* %107 to <4 x float>*
  store <4 x float> %110, <4 x float>* %111, align 4, !tbaa !316
  %112 = add nuw nsw i64 %6, 56
  %113 = getelementptr inbounds float, float* %4, i64 %112
  %114 = getelementptr inbounds float, float* %5, i64 %112
  %115 = bitcast float* %113 to <4 x float>*
  %116 = load <4 x float>, <4 x float>* %115, align 4, !tbaa !313
  %117 = fadd <4 x float> %14, %116
  %118 = bitcast float* %114 to <4 x float>*
  store <4 x float> %117, <4 x float>* %118, align 4, !tbaa !316
  %119 = add nuw nsw i64 %6, 60
  %120 = getelementptr inbounds float, float* %4, i64 %119
  %121 = getelementptr inbounds float, float* %5, i64 %119
  %122 = bitcast float* %120 to <4 x float>*
  %123 = load <4 x float>, <4 x float>* %122, align 4, !tbaa !313
  %124 = fadd <4 x float> %10, %123
  %125 = bitcast float* %121 to <4 x float>*
  store <4 x float> %124, <4 x float>* %125, align 4, !tbaa !316
  %126 = add nuw nsw i64 %6, 64
  %127 = getelementptr inbounds float, float* %4, i64 %126
  %128 = getelementptr inbounds float, float* %5, i64 %126
  %129 = bitcast float* %127 to <4 x float>*
  %130 = load <4 x float>, <4 x float>* %129, align 4, !tbaa !313
  %131 = fadd <4 x float> %12, %130
  %132 = bitcast float* %128 to <4 x float>*
  store <4 x float> %131, <4 x float>* %132, align 4, !tbaa !316
  %133 = add nuw nsw i64 %6, 68
  %134 = getelementptr inbounds float, float* %4, i64 %133
  %135 = getelementptr inbounds float, float* %5, i64 %133
  %136 = bitcast float* %134 to <4 x float>*
  %137 = load <4 x float>, <4 x float>* %136, align 4, !tbaa !313
  %138 = fadd <4 x float> %14, %137
  %139 = bitcast float* %135 to <4 x float>*
  store <4 x float> %138, <4 x float>* %139, align 4, !tbaa !316
  %140 = add nuw nsw i64 %6, 72
  %141 = getelementptr inbounds float, float* %4, i64 %140
  %142 = getelementptr inbounds float, float* %5, i64 %140
  %143 = bitcast float* %141 to <4 x float>*
  %144 = load <4 x float>, <4 x float>* %143, align 4, !tbaa !313
  %145 = fadd <4 x float> %10, %144
  %146 = bitcast float* %142 to <4 x float>*
  store <4 x float> %145, <4 x float>* %146, align 4, !tbaa !316
  %147 = add nuw nsw i64 %6, 76
  %148 = getelementptr inbounds float, float* %4, i64 %147
  %149 = getelementptr inbounds float, float* %5, i64 %147
  %150 = bitcast float* %148 to <4 x float>*
  %151 = load <4 x float>, <4 x float>* %150, align 4, !tbaa !313
  %152 = fadd <4 x float> %12, %151
  %153 = bitcast float* %149 to <4 x float>*
  store <4 x float> %152, <4 x float>* %153, align 4, !tbaa !316
  %154 = add nuw nsw i64 %6, 80
  %155 = getelementptr inbounds float, float* %4, i64 %154
  %156 = getelementptr inbounds float, float* %5, i64 %154
  %157 = bitcast float* %155 to <4 x float>*
  %158 = load <4 x float>, <4 x float>* %157, align 4, !tbaa !313
  %159 = fadd <4 x float> %14, %158
  %160 = bitcast float* %156 to <4 x float>*
  store <4 x float> %159, <4 x float>* %160, align 4, !tbaa !316
  %161 = add nuw nsw i64 %6, 84
  %162 = getelementptr inbounds float, float* %4, i64 %161
  %163 = getelementptr inbounds float, float* %5, i64 %161
  %164 = bitcast float* %162 to <4 x float>*
  %165 = load <4 x float>, <4 x float>* %164, align 4, !tbaa !313
  %166 = fadd <4 x float> %10, %165
  %167 = bitcast float* %163 to <4 x float>*
  store <4 x float> %166, <4 x float>* %167, align 4, !tbaa !316
  %168 = add nuw nsw i64 %6, 88
  %169 = getelementptr inbounds float, float* %4, i64 %168
  %170 = getelementptr inbounds float, float* %5, i64 %168
  %171 = bitcast float* %169 to <4 x float>*
  %172 = load <4 x float>, <4 x float>* %171, align 4, !tbaa !313
  %173 = fadd <4 x float> %12, %172
  %174 = bitcast float* %170 to <4 x float>*
  store <4 x float> %173, <4 x float>* %174, align 4, !tbaa !316
  %175 = add nuw nsw i64 %6, 92
  %176 = getelementptr inbounds float, float* %4, i64 %175
  %177 = getelementptr inbounds float, float* %5, i64 %175
  %178 = bitcast float* %176 to <4 x float>*
  %179 = load <4 x float>, <4 x float>* %178, align 4, !tbaa !313
  %180 = fadd <4 x float> %14, %179
  %181 = bitcast float* %177 to <4 x float>*
  store <4 x float> %180, <4 x float>* %181, align 4, !tbaa !316
  %182 = add nuw nsw i64 %6, 96
  %183 = getelementptr inbounds float, float* %4, i64 %182
  %184 = getelementptr inbounds float, float* %5, i64 %182
  %185 = bitcast float* %183 to <4 x float>*
  %186 = load <4 x float>, <4 x float>* %185, align 4, !tbaa !313
  %187 = fadd <4 x float> %10, %186
  %188 = bitcast float* %184 to <4 x float>*
  store <4 x float> %187, <4 x float>* %188, align 4, !tbaa !316
  %189 = add nuw nsw i64 %6, 100
  %190 = getelementptr inbounds float, float* %4, i64 %189
  %191 = getelementptr inbounds float, float* %5, i64 %189
  %192 = bitcast float* %190 to <4 x float>*
  %193 = load <4 x float>, <4 x float>* %192, align 4, !tbaa !313
  %194 = fadd <4 x float> %12, %193
  %195 = bitcast float* %191 to <4 x float>*
  store <4 x float> %194, <4 x float>* %195, align 4, !tbaa !316
  %196 = add nuw nsw i64 %6, 104
  %197 = getelementptr inbounds float, float* %4, i64 %196
  %198 = getelementptr inbounds float, float* %5, i64 %196
  %199 = bitcast float* %197 to <4 x float>*
  %200 = load <4 x float>, <4 x float>* %199, align 4, !tbaa !313
  %201 = fadd <4 x float> %14, %200
  %202 = bitcast float* %198 to <4 x float>*
  store <4 x float> %201, <4 x float>* %202, align 4, !tbaa !316
  %203 = add nuw nsw i64 %6, 108
  %204 = getelementptr inbounds float, float* %4, i64 %203
  %205 = getelementptr inbounds float, float* %5, i64 %203
  %206 = bitcast float* %204 to <4 x float>*
  %207 = load <4 x float>, <4 x float>* %206, align 4, !tbaa !313
  %208 = fadd <4 x float> %10, %207
  %209 = bitcast float* %205 to <4 x float>*
  store <4 x float> %208, <4 x float>* %209, align 4, !tbaa !316
  %210 = add nuw nsw i64 %6, 112
  %211 = getelementptr inbounds float, float* %4, i64 %210
  %212 = getelementptr inbounds float, float* %5, i64 %210
  %213 = bitcast float* %211 to <4 x float>*
  %214 = load <4 x float>, <4 x float>* %213, align 4, !tbaa !313
  %215 = fadd <4 x float> %12, %214
  %216 = bitcast float* %212 to <4 x float>*
  store <4 x float> %215, <4 x float>* %216, align 4, !tbaa !316
  %217 = add nuw nsw i64 %6, 116
  %218 = getelementptr inbounds float, float* %4, i64 %217
  %219 = getelementptr inbounds float, float* %5, i64 %217
  %220 = bitcast float* %218 to <4 x float>*
  %221 = load <4 x float>, <4 x float>* %220, align 4, !tbaa !313
  %222 = fadd <4 x float> %14, %221
  %223 = bitcast float* %219 to <4 x float>*
  store <4 x float> %222, <4 x float>* %223, align 4, !tbaa !316
  %224 = add nuw nsw i64 %6, 120
  %225 = getelementptr inbounds float, float* %4, i64 %224
  %226 = getelementptr inbounds float, float* %5, i64 %224
  %227 = bitcast float* %225 to <4 x float>*
  %228 = load <4 x float>, <4 x float>* %227, align 4, !tbaa !313
  %229 = fadd <4 x float> %10, %228
  %230 = bitcast float* %226 to <4 x float>*
  store <4 x float> %229, <4 x float>* %230, align 4, !tbaa !316
  %231 = add nuw nsw i64 %6, 124
  %232 = getelementptr inbounds float, float* %4, i64 %231
  %233 = getelementptr inbounds float, float* %5, i64 %231
  %234 = bitcast float* %232 to <4 x float>*
  %235 = load <4 x float>, <4 x float>* %234, align 4, !tbaa !313
  %236 = fadd <4 x float> %12, %235
  %237 = bitcast float* %233 to <4 x float>*
  store <4 x float> %236, <4 x float>* %237, align 4, !tbaa !316
  %238 = add nuw nsw i64 %6, 128
  %239 = getelementptr inbounds float, float* %4, i64 %238
  %240 = getelementptr inbounds float, float* %5, i64 %238
  %241 = bitcast float* %239 to <4 x float>*
  %242 = load <4 x float>, <4 x float>* %241, align 4, !tbaa !313
  %243 = fadd <4 x float> %14, %242
  %244 = bitcast float* %240 to <4 x float>*
  store <4 x float> %243, <4 x float>* %244, align 4, !tbaa !316
  %245 = add nuw nsw i64 %6, 132
  %246 = getelementptr inbounds float, float* %4, i64 %245
  %247 = getelementptr inbounds float, float* %5, i64 %245
  %248 = bitcast float* %246 to <4 x float>*
  %249 = load <4 x float>, <4 x float>* %248, align 4, !tbaa !313
  %250 = fadd <4 x float> %10, %249
  %251 = bitcast float* %247 to <4 x float>*
  store <4 x float> %250, <4 x float>* %251, align 4, !tbaa !316
  %252 = add nuw nsw i64 %6, 136
  %253 = getelementptr inbounds float, float* %4, i64 %252
  %254 = getelementptr inbounds float, float* %5, i64 %252
  %255 = bitcast float* %253 to <4 x float>*
  %256 = load <4 x float>, <4 x float>* %255, align 4, !tbaa !313
  %257 = fadd <4 x float> %12, %256
  %258 = bitcast float* %254 to <4 x float>*
  store <4 x float> %257, <4 x float>* %258, align 4, !tbaa !316
  %259 = add nuw nsw i64 %6, 140
  %260 = getelementptr inbounds float, float* %4, i64 %259
  %261 = getelementptr inbounds float, float* %5, i64 %259
  %262 = bitcast float* %260 to <4 x float>*
  %263 = load <4 x float>, <4 x float>* %262, align 4, !tbaa !313
  %264 = fadd <4 x float> %14, %263
  %265 = bitcast float* %261 to <4 x float>*
  store <4 x float> %264, <4 x float>* %265, align 4, !tbaa !316
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 256
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !28

for_end:                                          ; preds = %for_begin1.preheader
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
  %wide.load = load <4 x float>, <4 x float>* %5, align 4, !tbaa !319
  %6 = fsub <4 x float> %wide.load, %broadcast.splat16
  %7 = call <4 x float> @llvm.exp.v4f32(<4 x float> %6)
  %8 = getelementptr inbounds [1000 x float], [1000 x float]* %2, i64 0, i64 %index
  %9 = bitcast float* %8 to <4 x float>*
  store <4 x float> %7, <4 x float>* %9, align 16, !tbaa !322
  %index.next = add i64 %index, 4
  %10 = icmp eq i64 %index.next, 1000
  br i1 %10, label %for_body5, label %vector.body, !llvm.loop !325

for_body:                                         ; preds = %for_body, %entry
  %indvars.iv10 = phi i64 [ 0, %entry ], [ %indvars.iv.next11, %for_body ]
  %.02 = phi float [ 0xC7EFFFFFE0000000, %entry ], [ %14, %for_body ]
  %11 = getelementptr inbounds float, float* %3, i64 %indvars.iv10
  %12 = load float, float* %11, align 4, !tbaa !319
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
  %wide.load28 = load <4 x float>, <4 x float>* %17, align 16, !tbaa !322
  %18 = fdiv <4 x float> %wide.load28, %broadcast.splat30
  %19 = getelementptr inbounds float, float* %15, i64 %index21
  %20 = bitcast float* %19 to <4 x float>*
  store <4 x float> %18, <4 x float>* %20, align 4, !tbaa !326
  %index.next22 = add i64 %index21, 4
  %21 = icmp eq i64 %index.next22, 1000
  br i1 %21, label %for_end9, label %vector.body17, !llvm.loop !329

for_body5:                                        ; preds = %vector.body, %for_body5
  %indvars.iv4 = phi i64 [ %indvars.iv.next5, %for_body5 ], [ 0, %vector.body ]
  %.0131 = phi float [ %24, %for_body5 ], [ 0.000000e+00, %vector.body ]
  %22 = getelementptr inbounds [1000 x float], [1000 x float]* %2, i64 0, i64 %indvars.iv4
  %23 = load float, float* %22, align 4, !tbaa !322
  %24 = fadd float %.0131, %23
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 1000
  br i1 %exitcond6, label %for_begin7.preheader, label %for_body5, !prof !28

for_end9:                                         ; preds = %vector.body17
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
  %indvars.iv5 = phi i64 [ 0, %entry ], [ %indvars.iv.next6, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv5, 25
  %5 = mul nuw nsw i64 %indvars.iv5, 144
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_begin4.preheader ]
  %6 = mul nuw nsw i64 %indvars.iv, 5
  %7 = add nuw nsw i64 %6, %4
  %8 = mul nuw nsw i64 %indvars.iv, 24
  %9 = add nuw nsw i64 %8, %5
  %10 = getelementptr inbounds float, float* %2, i64 %7
  %11 = getelementptr inbounds float, float* %3, i64 %9
  %12 = load float, float* %11, align 4, !tbaa !330
  %13 = fcmp olt float %12, 0xC7EFFFFFE0000000
  %14 = select i1 %13, float 0xC7EFFFFFE0000000, float %12
  %15 = or i64 %9, 1
  %16 = getelementptr inbounds float, float* %3, i64 %15
  %17 = load float, float* %16, align 4, !tbaa !330
  %18 = fcmp ogt float %14, %17
  %19 = select i1 %18, float %14, float %17
  %20 = or i64 %9, 2
  %21 = getelementptr inbounds float, float* %3, i64 %20
  %22 = load float, float* %21, align 4, !tbaa !330
  %23 = fcmp ogt float %19, %22
  %24 = select i1 %23, float %19, float %22
  %25 = add nuw nsw i64 %9, 12
  %26 = getelementptr inbounds float, float* %3, i64 %25
  %27 = load float, float* %26, align 4, !tbaa !330
  %28 = fcmp ogt float %24, %27
  %29 = select i1 %28, float %24, float %27
  %30 = add nuw nsw i64 %9, 13
  %31 = getelementptr inbounds float, float* %3, i64 %30
  %32 = load float, float* %31, align 4, !tbaa !330
  %33 = fcmp ogt float %29, %32
  %34 = select i1 %33, float %29, float %32
  %35 = add nuw nsw i64 %9, 14
  %36 = getelementptr inbounds float, float* %3, i64 %35
  %37 = load float, float* %36, align 4, !tbaa !330
  %38 = fcmp ogt float %34, %37
  %39 = select i1 %38, float %34, float %37
  %40 = add nuw nsw i64 %9, 24
  %41 = getelementptr inbounds float, float* %3, i64 %40
  %42 = load float, float* %41, align 4, !tbaa !330
  %43 = fcmp ogt float %39, %42
  %44 = select i1 %43, float %39, float %42
  %45 = add nuw nsw i64 %9, 25
  %46 = getelementptr inbounds float, float* %3, i64 %45
  %47 = load float, float* %46, align 4, !tbaa !330
  %48 = fcmp ogt float %44, %47
  %49 = select i1 %48, float %44, float %47
  %50 = add nuw nsw i64 %9, 26
  %51 = getelementptr inbounds float, float* %3, i64 %50
  %52 = load float, float* %51, align 4, !tbaa !330
  %53 = fcmp ogt float %49, %52
  %54 = select i1 %53, float %49, float %52
  store float %54, float* %10, align 4, !tbaa !333
  %55 = add nuw nsw i64 %7, 1
  %56 = getelementptr inbounds float, float* %2, i64 %55
  %57 = fcmp olt float %22, 0xC7EFFFFFE0000000
  %58 = select i1 %57, float 0xC7EFFFFFE0000000, float %22
  %59 = or i64 %9, 3
  %60 = getelementptr inbounds float, float* %3, i64 %59
  %61 = load float, float* %60, align 4, !tbaa !330
  %62 = fcmp ogt float %58, %61
  %63 = select i1 %62, float %58, float %61
  %64 = add nuw nsw i64 %20, 2
  %65 = getelementptr inbounds float, float* %3, i64 %64
  %66 = load float, float* %65, align 4, !tbaa !330
  %67 = fcmp ogt float %63, %66
  %68 = select i1 %67, float %63, float %66
  %69 = fcmp ogt float %68, %37
  %70 = select i1 %69, float %68, float %37
  %71 = add nuw nsw i64 %20, 13
  %72 = getelementptr inbounds float, float* %3, i64 %71
  %73 = load float, float* %72, align 4, !tbaa !330
  %74 = fcmp ogt float %70, %73
  %75 = select i1 %74, float %70, float %73
  %76 = add nuw nsw i64 %20, 14
  %77 = getelementptr inbounds float, float* %3, i64 %76
  %78 = load float, float* %77, align 4, !tbaa !330
  %79 = fcmp ogt float %75, %78
  %80 = select i1 %79, float %75, float %78
  %81 = fcmp ogt float %80, %52
  %82 = select i1 %81, float %80, float %52
  %83 = add nuw nsw i64 %20, 25
  %84 = getelementptr inbounds float, float* %3, i64 %83
  %85 = load float, float* %84, align 4, !tbaa !330
  %86 = fcmp ogt float %82, %85
  %87 = select i1 %86, float %82, float %85
  %88 = add nuw nsw i64 %20, 26
  %89 = getelementptr inbounds float, float* %3, i64 %88
  %90 = load float, float* %89, align 4, !tbaa !330
  %91 = fcmp ogt float %87, %90
  %92 = select i1 %91, float %87, float %90
  store float %92, float* %56, align 4, !tbaa !333
  %93 = add nuw nsw i64 %7, 2
  %94 = getelementptr inbounds float, float* %2, i64 %93
  %95 = or i64 %9, 4
  %96 = fcmp olt float %66, 0xC7EFFFFFE0000000
  %97 = select i1 %96, float 0xC7EFFFFFE0000000, float %66
  %98 = or i64 %9, 5
  %99 = getelementptr inbounds float, float* %3, i64 %98
  %100 = load float, float* %99, align 4, !tbaa !330
  %101 = fcmp ogt float %97, %100
  %102 = select i1 %101, float %97, float %100
  %103 = or i64 %9, 6
  %104 = getelementptr inbounds float, float* %3, i64 %103
  %105 = load float, float* %104, align 4, !tbaa !330
  %106 = fcmp ogt float %102, %105
  %107 = select i1 %106, float %102, float %105
  %108 = fcmp ogt float %107, %78
  %109 = select i1 %108, float %107, float %78
  %110 = add nuw nsw i64 %95, 13
  %111 = getelementptr inbounds float, float* %3, i64 %110
  %112 = load float, float* %111, align 4, !tbaa !330
  %113 = fcmp ogt float %109, %112
  %114 = select i1 %113, float %109, float %112
  %115 = add nuw nsw i64 %95, 14
  %116 = getelementptr inbounds float, float* %3, i64 %115
  %117 = load float, float* %116, align 4, !tbaa !330
  %118 = fcmp ogt float %114, %117
  %119 = select i1 %118, float %114, float %117
  %120 = fcmp ogt float %119, %90
  %121 = select i1 %120, float %119, float %90
  %122 = add nuw nsw i64 %95, 25
  %123 = getelementptr inbounds float, float* %3, i64 %122
  %124 = load float, float* %123, align 4, !tbaa !330
  %125 = fcmp ogt float %121, %124
  %126 = select i1 %125, float %121, float %124
  %127 = add nuw nsw i64 %95, 26
  %128 = getelementptr inbounds float, float* %3, i64 %127
  %129 = load float, float* %128, align 4, !tbaa !330
  %130 = fcmp ogt float %126, %129
  %131 = select i1 %130, float %126, float %129
  store float %131, float* %94, align 4, !tbaa !333
  %132 = add nuw nsw i64 %7, 3
  %133 = getelementptr inbounds float, float* %2, i64 %132
  %134 = or i64 %9, 6
  %135 = fcmp olt float %105, 0xC7EFFFFFE0000000
  %136 = select i1 %135, float 0xC7EFFFFFE0000000, float %105
  %137 = or i64 %9, 7
  %138 = getelementptr inbounds float, float* %3, i64 %137
  %139 = load float, float* %138, align 4, !tbaa !330
  %140 = fcmp ogt float %136, %139
  %141 = select i1 %140, float %136, float %139
  %142 = add nuw nsw i64 %134, 2
  %143 = getelementptr inbounds float, float* %3, i64 %142
  %144 = load float, float* %143, align 4, !tbaa !330
  %145 = fcmp ogt float %141, %144
  %146 = select i1 %145, float %141, float %144
  %147 = fcmp ogt float %146, %117
  %148 = select i1 %147, float %146, float %117
  %149 = add nuw nsw i64 %134, 13
  %150 = getelementptr inbounds float, float* %3, i64 %149
  %151 = load float, float* %150, align 4, !tbaa !330
  %152 = fcmp ogt float %148, %151
  %153 = select i1 %152, float %148, float %151
  %154 = add nuw nsw i64 %134, 14
  %155 = getelementptr inbounds float, float* %3, i64 %154
  %156 = load float, float* %155, align 4, !tbaa !330
  %157 = fcmp ogt float %153, %156
  %158 = select i1 %157, float %153, float %156
  %159 = fcmp ogt float %158, %129
  %160 = select i1 %159, float %158, float %129
  %161 = add nuw nsw i64 %134, 25
  %162 = getelementptr inbounds float, float* %3, i64 %161
  %163 = load float, float* %162, align 4, !tbaa !330
  %164 = fcmp ogt float %160, %163
  %165 = select i1 %164, float %160, float %163
  %166 = add nuw nsw i64 %134, 26
  %167 = getelementptr inbounds float, float* %3, i64 %166
  %168 = load float, float* %167, align 4, !tbaa !330
  %169 = fcmp ogt float %165, %168
  %170 = select i1 %169, float %165, float %168
  store float %170, float* %133, align 4, !tbaa !333
  %171 = add nuw nsw i64 %7, 4
  %172 = getelementptr inbounds float, float* %2, i64 %171
  %173 = fcmp olt float %144, 0xC7EFFFFFE0000000
  %174 = select i1 %173, float 0xC7EFFFFFE0000000, float %144
  %175 = add nuw nsw i64 %9, 9
  %176 = getelementptr inbounds float, float* %3, i64 %175
  %177 = load float, float* %176, align 4, !tbaa !330
  %178 = fcmp ogt float %174, %177
  %179 = select i1 %178, float %174, float %177
  %180 = add nuw nsw i64 %9, 10
  %181 = getelementptr inbounds float, float* %3, i64 %180
  %182 = load float, float* %181, align 4, !tbaa !330
  %183 = fcmp ogt float %179, %182
  %184 = select i1 %183, float %179, float %182
  %185 = fcmp ogt float %184, %156
  %186 = select i1 %185, float %184, float %156
  %187 = add nuw nsw i64 %9, 21
  %188 = getelementptr inbounds float, float* %3, i64 %187
  %189 = load float, float* %188, align 4, !tbaa !330
  %190 = fcmp ogt float %186, %189
  %191 = select i1 %190, float %186, float %189
  %192 = add nuw nsw i64 %9, 22
  %193 = getelementptr inbounds float, float* %3, i64 %192
  %194 = load float, float* %193, align 4, !tbaa !330
  %195 = fcmp ogt float %191, %194
  %196 = select i1 %195, float %191, float %194
  %197 = fcmp ogt float %196, %168
  %198 = select i1 %197, float %196, float %168
  %199 = add nuw nsw i64 %9, 33
  %200 = getelementptr inbounds float, float* %3, i64 %199
  %201 = load float, float* %200, align 4, !tbaa !330
  %202 = fcmp ogt float %198, %201
  %203 = select i1 %202, float %198, float %201
  %204 = add nuw nsw i64 %9, 34
  %205 = getelementptr inbounds float, float* %3, i64 %204
  %206 = load float, float* %205, align 4, !tbaa !330
  %207 = fcmp ogt float %203, %206
  %208 = select i1 %207, float %203, float %206
  store float %208, float* %172, align 4, !tbaa !333
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 5
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !28

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next6 = add nuw nsw i64 %indvars.iv5, 1
  %exitcond7 = icmp eq i64 %indvars.iv.next6, 256
  br i1 %exitcond7, label %for_end, label %for_begin1.preheader, !prof !28
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
  %8 = load <4 x float>, <4 x float>* %7, align 4, !tbaa !336
  %9 = fcmp ogt <4 x float> %8, zeroinitializer
  %10 = select <4 x i1> %9, <4 x float> %8, <4 x float> zeroinitializer
  %11 = bitcast float* %6 to <4 x float>*
  store <4 x float> %10, <4 x float>* %11, align 4, !tbaa !339
  %12 = or i64 %4, 4
  %13 = getelementptr inbounds float, float* %2, i64 %12
  %14 = getelementptr inbounds float, float* %3, i64 %12
  %15 = bitcast float* %13 to <4 x float>*
  %16 = load <4 x float>, <4 x float>* %15, align 4, !tbaa !336
  %17 = fcmp ogt <4 x float> %16, zeroinitializer
  %18 = select <4 x i1> %17, <4 x float> %16, <4 x float> zeroinitializer
  %19 = bitcast float* %14 to <4 x float>*
  store <4 x float> %18, <4 x float>* %19, align 4, !tbaa !339
  %20 = or i64 %4, 8
  %21 = getelementptr inbounds float, float* %2, i64 %20
  %22 = getelementptr inbounds float, float* %3, i64 %20
  %23 = bitcast float* %21 to <4 x float>*
  %24 = load <4 x float>, <4 x float>* %23, align 4, !tbaa !336
  %25 = fcmp ogt <4 x float> %24, zeroinitializer
  %26 = select <4 x i1> %25, <4 x float> %24, <4 x float> zeroinitializer
  %27 = bitcast float* %22 to <4 x float>*
  store <4 x float> %26, <4 x float>* %27, align 4, !tbaa !339
  %28 = or i64 %4, 12
  %29 = getelementptr inbounds float, float* %2, i64 %28
  %30 = getelementptr inbounds float, float* %3, i64 %28
  %31 = bitcast float* %29 to <4 x float>*
  %32 = load <4 x float>, <4 x float>* %31, align 4, !tbaa !336
  %33 = fcmp ogt <4 x float> %32, zeroinitializer
  %34 = select <4 x i1> %33, <4 x float> %32, <4 x float> zeroinitializer
  %35 = bitcast float* %30 to <4 x float>*
  store <4 x float> %34, <4 x float>* %35, align 4, !tbaa !339
  %36 = add nuw nsw i64 %28, 4
  %37 = getelementptr inbounds float, float* %2, i64 %36
  %38 = getelementptr inbounds float, float* %3, i64 %36
  %39 = bitcast float* %37 to <4 x float>*
  %40 = load <4 x float>, <4 x float>* %39, align 4, !tbaa !336
  %41 = fcmp ogt <4 x float> %40, zeroinitializer
  %42 = select <4 x i1> %41, <4 x float> %40, <4 x float> zeroinitializer
  %43 = bitcast float* %38 to <4 x float>*
  store <4 x float> %42, <4 x float>* %43, align 4, !tbaa !339
  %44 = add nuw nsw i64 %28, 8
  %45 = getelementptr inbounds float, float* %2, i64 %44
  %46 = getelementptr inbounds float, float* %3, i64 %44
  %47 = bitcast float* %45 to <4 x float>*
  %48 = load <4 x float>, <4 x float>* %47, align 4, !tbaa !336
  %49 = fcmp ogt <4 x float> %48, zeroinitializer
  %50 = select <4 x i1> %49, <4 x float> %48, <4 x float> zeroinitializer
  %51 = bitcast float* %46 to <4 x float>*
  store <4 x float> %50, <4 x float>* %51, align 4, !tbaa !339
  %52 = add nuw nsw i64 %4, 24
  %53 = getelementptr inbounds float, float* %2, i64 %52
  %54 = getelementptr inbounds float, float* %3, i64 %52
  %55 = bitcast float* %53 to <4 x float>*
  %56 = load <4 x float>, <4 x float>* %55, align 4, !tbaa !336
  %57 = fcmp ogt <4 x float> %56, zeroinitializer
  %58 = select <4 x i1> %57, <4 x float> %56, <4 x float> zeroinitializer
  %59 = bitcast float* %54 to <4 x float>*
  store <4 x float> %58, <4 x float>* %59, align 4, !tbaa !339
  %60 = add nuw nsw i64 %4, 28
  %61 = getelementptr inbounds float, float* %2, i64 %60
  %62 = getelementptr inbounds float, float* %3, i64 %60
  %63 = bitcast float* %61 to <4 x float>*
  %64 = load <4 x float>, <4 x float>* %63, align 4, !tbaa !336
  %65 = fcmp ogt <4 x float> %64, zeroinitializer
  %66 = select <4 x i1> %65, <4 x float> %64, <4 x float> zeroinitializer
  %67 = bitcast float* %62 to <4 x float>*
  store <4 x float> %66, <4 x float>* %67, align 4, !tbaa !339
  %68 = add nuw nsw i64 %4, 32
  %69 = getelementptr inbounds float, float* %2, i64 %68
  %70 = getelementptr inbounds float, float* %3, i64 %68
  %71 = bitcast float* %69 to <4 x float>*
  %72 = load <4 x float>, <4 x float>* %71, align 4, !tbaa !336
  %73 = fcmp ogt <4 x float> %72, zeroinitializer
  %74 = select <4 x i1> %73, <4 x float> %72, <4 x float> zeroinitializer
  %75 = bitcast float* %70 to <4 x float>*
  store <4 x float> %74, <4 x float>* %75, align 4, !tbaa !339
  %76 = add nuw nsw i64 %4, 36
  %77 = getelementptr inbounds float, float* %2, i64 %76
  %78 = getelementptr inbounds float, float* %3, i64 %76
  %79 = bitcast float* %77 to <4 x float>*
  %80 = load <4 x float>, <4 x float>* %79, align 4, !tbaa !336
  %81 = fcmp ogt <4 x float> %80, zeroinitializer
  %82 = select <4 x i1> %81, <4 x float> %80, <4 x float> zeroinitializer
  %83 = bitcast float* %78 to <4 x float>*
  store <4 x float> %82, <4 x float>* %83, align 4, !tbaa !339
  %84 = add nuw nsw i64 %4, 40
  %85 = getelementptr inbounds float, float* %2, i64 %84
  %86 = getelementptr inbounds float, float* %3, i64 %84
  %87 = bitcast float* %85 to <4 x float>*
  %88 = load <4 x float>, <4 x float>* %87, align 4, !tbaa !336
  %89 = fcmp ogt <4 x float> %88, zeroinitializer
  %90 = select <4 x i1> %89, <4 x float> %88, <4 x float> zeroinitializer
  %91 = bitcast float* %86 to <4 x float>*
  store <4 x float> %90, <4 x float>* %91, align 4, !tbaa !339
  %92 = add nuw nsw i64 %4, 44
  %93 = getelementptr inbounds float, float* %2, i64 %92
  %94 = getelementptr inbounds float, float* %3, i64 %92
  %95 = bitcast float* %93 to <4 x float>*
  %96 = load <4 x float>, <4 x float>* %95, align 4, !tbaa !336
  %97 = fcmp ogt <4 x float> %96, zeroinitializer
  %98 = select <4 x i1> %97, <4 x float> %96, <4 x float> zeroinitializer
  %99 = bitcast float* %94 to <4 x float>*
  store <4 x float> %98, <4 x float>* %99, align 4, !tbaa !339
  %100 = add nuw nsw i64 %4, 48
  %101 = getelementptr inbounds float, float* %2, i64 %100
  %102 = getelementptr inbounds float, float* %3, i64 %100
  %103 = bitcast float* %101 to <4 x float>*
  %104 = load <4 x float>, <4 x float>* %103, align 4, !tbaa !336
  %105 = fcmp ogt <4 x float> %104, zeroinitializer
  %106 = select <4 x i1> %105, <4 x float> %104, <4 x float> zeroinitializer
  %107 = bitcast float* %102 to <4 x float>*
  store <4 x float> %106, <4 x float>* %107, align 4, !tbaa !339
  %108 = add nuw nsw i64 %4, 52
  %109 = getelementptr inbounds float, float* %2, i64 %108
  %110 = getelementptr inbounds float, float* %3, i64 %108
  %111 = bitcast float* %109 to <4 x float>*
  %112 = load <4 x float>, <4 x float>* %111, align 4, !tbaa !336
  %113 = fcmp ogt <4 x float> %112, zeroinitializer
  %114 = select <4 x i1> %113, <4 x float> %112, <4 x float> zeroinitializer
  %115 = bitcast float* %110 to <4 x float>*
  store <4 x float> %114, <4 x float>* %115, align 4, !tbaa !339
  %116 = add nuw nsw i64 %4, 56
  %117 = getelementptr inbounds float, float* %2, i64 %116
  %118 = getelementptr inbounds float, float* %3, i64 %116
  %119 = bitcast float* %117 to <4 x float>*
  %120 = load <4 x float>, <4 x float>* %119, align 4, !tbaa !336
  %121 = fcmp ogt <4 x float> %120, zeroinitializer
  %122 = select <4 x i1> %121, <4 x float> %120, <4 x float> zeroinitializer
  %123 = bitcast float* %118 to <4 x float>*
  store <4 x float> %122, <4 x float>* %123, align 4, !tbaa !339
  %124 = add nuw nsw i64 %4, 60
  %125 = getelementptr inbounds float, float* %2, i64 %124
  %126 = getelementptr inbounds float, float* %3, i64 %124
  %127 = bitcast float* %125 to <4 x float>*
  %128 = load <4 x float>, <4 x float>* %127, align 4, !tbaa !336
  %129 = fcmp ogt <4 x float> %128, zeroinitializer
  %130 = select <4 x i1> %129, <4 x float> %128, <4 x float> zeroinitializer
  %131 = bitcast float* %126 to <4 x float>*
  store <4 x float> %130, <4 x float>* %131, align 4, !tbaa !339
  %132 = add nuw nsw i64 %4, 64
  %133 = getelementptr inbounds float, float* %2, i64 %132
  %134 = getelementptr inbounds float, float* %3, i64 %132
  %135 = bitcast float* %133 to <4 x float>*
  %136 = load <4 x float>, <4 x float>* %135, align 4, !tbaa !336
  %137 = fcmp ogt <4 x float> %136, zeroinitializer
  %138 = select <4 x i1> %137, <4 x float> %136, <4 x float> zeroinitializer
  %139 = bitcast float* %134 to <4 x float>*
  store <4 x float> %138, <4 x float>* %139, align 4, !tbaa !339
  %140 = add nuw nsw i64 %4, 68
  %141 = getelementptr inbounds float, float* %2, i64 %140
  %142 = getelementptr inbounds float, float* %3, i64 %140
  %143 = bitcast float* %141 to <4 x float>*
  %144 = load <4 x float>, <4 x float>* %143, align 4, !tbaa !336
  %145 = fcmp ogt <4 x float> %144, zeroinitializer
  %146 = select <4 x i1> %145, <4 x float> %144, <4 x float> zeroinitializer
  %147 = bitcast float* %142 to <4 x float>*
  store <4 x float> %146, <4 x float>* %147, align 4, !tbaa !339
  %148 = add nuw nsw i64 %4, 72
  %149 = getelementptr inbounds float, float* %2, i64 %148
  %150 = getelementptr inbounds float, float* %3, i64 %148
  %151 = bitcast float* %149 to <4 x float>*
  %152 = load <4 x float>, <4 x float>* %151, align 4, !tbaa !336
  %153 = fcmp ogt <4 x float> %152, zeroinitializer
  %154 = select <4 x i1> %153, <4 x float> %152, <4 x float> zeroinitializer
  %155 = bitcast float* %150 to <4 x float>*
  store <4 x float> %154, <4 x float>* %155, align 4, !tbaa !339
  %156 = add nuw nsw i64 %4, 76
  %157 = getelementptr inbounds float, float* %2, i64 %156
  %158 = getelementptr inbounds float, float* %3, i64 %156
  %159 = bitcast float* %157 to <4 x float>*
  %160 = load <4 x float>, <4 x float>* %159, align 4, !tbaa !336
  %161 = fcmp ogt <4 x float> %160, zeroinitializer
  %162 = select <4 x i1> %161, <4 x float> %160, <4 x float> zeroinitializer
  %163 = bitcast float* %158 to <4 x float>*
  store <4 x float> %162, <4 x float>* %163, align 4, !tbaa !339
  %164 = add nuw nsw i64 %4, 80
  %165 = getelementptr inbounds float, float* %2, i64 %164
  %166 = getelementptr inbounds float, float* %3, i64 %164
  %167 = bitcast float* %165 to <4 x float>*
  %168 = load <4 x float>, <4 x float>* %167, align 4, !tbaa !336
  %169 = fcmp ogt <4 x float> %168, zeroinitializer
  %170 = select <4 x i1> %169, <4 x float> %168, <4 x float> zeroinitializer
  %171 = bitcast float* %166 to <4 x float>*
  store <4 x float> %170, <4 x float>* %171, align 4, !tbaa !339
  %172 = add nuw nsw i64 %4, 84
  %173 = getelementptr inbounds float, float* %2, i64 %172
  %174 = getelementptr inbounds float, float* %3, i64 %172
  %175 = bitcast float* %173 to <4 x float>*
  %176 = load <4 x float>, <4 x float>* %175, align 4, !tbaa !336
  %177 = fcmp ogt <4 x float> %176, zeroinitializer
  %178 = select <4 x i1> %177, <4 x float> %176, <4 x float> zeroinitializer
  %179 = bitcast float* %174 to <4 x float>*
  store <4 x float> %178, <4 x float>* %179, align 4, !tbaa !339
  %180 = add nuw nsw i64 %4, 88
  %181 = getelementptr inbounds float, float* %2, i64 %180
  %182 = getelementptr inbounds float, float* %3, i64 %180
  %183 = bitcast float* %181 to <4 x float>*
  %184 = load <4 x float>, <4 x float>* %183, align 4, !tbaa !336
  %185 = fcmp ogt <4 x float> %184, zeroinitializer
  %186 = select <4 x i1> %185, <4 x float> %184, <4 x float> zeroinitializer
  %187 = bitcast float* %182 to <4 x float>*
  store <4 x float> %186, <4 x float>* %187, align 4, !tbaa !339
  %188 = add nuw nsw i64 %4, 92
  %189 = getelementptr inbounds float, float* %2, i64 %188
  %190 = getelementptr inbounds float, float* %3, i64 %188
  %191 = bitcast float* %189 to <4 x float>*
  %192 = load <4 x float>, <4 x float>* %191, align 4, !tbaa !336
  %193 = fcmp ogt <4 x float> %192, zeroinitializer
  %194 = select <4 x i1> %193, <4 x float> %192, <4 x float> zeroinitializer
  %195 = bitcast float* %190 to <4 x float>*
  store <4 x float> %194, <4 x float>* %195, align 4, !tbaa !339
  %196 = add nuw nsw i64 %4, 96
  %197 = getelementptr inbounds float, float* %2, i64 %196
  %198 = getelementptr inbounds float, float* %3, i64 %196
  %199 = bitcast float* %197 to <4 x float>*
  %200 = load <4 x float>, <4 x float>* %199, align 4, !tbaa !336
  %201 = fcmp ogt <4 x float> %200, zeroinitializer
  %202 = select <4 x i1> %201, <4 x float> %200, <4 x float> zeroinitializer
  %203 = bitcast float* %198 to <4 x float>*
  store <4 x float> %202, <4 x float>* %203, align 4, !tbaa !339
  %204 = add nuw nsw i64 %4, 100
  %205 = getelementptr inbounds float, float* %2, i64 %204
  %206 = getelementptr inbounds float, float* %3, i64 %204
  %207 = bitcast float* %205 to <4 x float>*
  %208 = load <4 x float>, <4 x float>* %207, align 4, !tbaa !336
  %209 = fcmp ogt <4 x float> %208, zeroinitializer
  %210 = select <4 x i1> %209, <4 x float> %208, <4 x float> zeroinitializer
  %211 = bitcast float* %206 to <4 x float>*
  store <4 x float> %210, <4 x float>* %211, align 4, !tbaa !339
  %212 = add nuw nsw i64 %4, 104
  %213 = getelementptr inbounds float, float* %2, i64 %212
  %214 = getelementptr inbounds float, float* %3, i64 %212
  %215 = bitcast float* %213 to <4 x float>*
  %216 = load <4 x float>, <4 x float>* %215, align 4, !tbaa !336
  %217 = fcmp ogt <4 x float> %216, zeroinitializer
  %218 = select <4 x i1> %217, <4 x float> %216, <4 x float> zeroinitializer
  %219 = bitcast float* %214 to <4 x float>*
  store <4 x float> %218, <4 x float>* %219, align 4, !tbaa !339
  %220 = add nuw nsw i64 %4, 108
  %221 = getelementptr inbounds float, float* %2, i64 %220
  %222 = getelementptr inbounds float, float* %3, i64 %220
  %223 = bitcast float* %221 to <4 x float>*
  %224 = load <4 x float>, <4 x float>* %223, align 4, !tbaa !336
  %225 = fcmp ogt <4 x float> %224, zeroinitializer
  %226 = select <4 x i1> %225, <4 x float> %224, <4 x float> zeroinitializer
  %227 = bitcast float* %222 to <4 x float>*
  store <4 x float> %226, <4 x float>* %227, align 4, !tbaa !339
  %228 = add nuw nsw i64 %4, 112
  %229 = getelementptr inbounds float, float* %2, i64 %228
  %230 = getelementptr inbounds float, float* %3, i64 %228
  %231 = bitcast float* %229 to <4 x float>*
  %232 = load <4 x float>, <4 x float>* %231, align 4, !tbaa !336
  %233 = fcmp ogt <4 x float> %232, zeroinitializer
  %234 = select <4 x i1> %233, <4 x float> %232, <4 x float> zeroinitializer
  %235 = bitcast float* %230 to <4 x float>*
  store <4 x float> %234, <4 x float>* %235, align 4, !tbaa !339
  %236 = add nuw nsw i64 %4, 116
  %237 = getelementptr inbounds float, float* %2, i64 %236
  %238 = getelementptr inbounds float, float* %3, i64 %236
  %239 = bitcast float* %237 to <4 x float>*
  %240 = load <4 x float>, <4 x float>* %239, align 4, !tbaa !336
  %241 = fcmp ogt <4 x float> %240, zeroinitializer
  %242 = select <4 x i1> %241, <4 x float> %240, <4 x float> zeroinitializer
  %243 = bitcast float* %238 to <4 x float>*
  store <4 x float> %242, <4 x float>* %243, align 4, !tbaa !339
  %244 = add nuw nsw i64 %4, 120
  %245 = getelementptr inbounds float, float* %2, i64 %244
  %246 = getelementptr inbounds float, float* %3, i64 %244
  %247 = bitcast float* %245 to <4 x float>*
  %248 = load <4 x float>, <4 x float>* %247, align 4, !tbaa !336
  %249 = fcmp ogt <4 x float> %248, zeroinitializer
  %250 = select <4 x i1> %249, <4 x float> %248, <4 x float> zeroinitializer
  %251 = bitcast float* %246 to <4 x float>*
  store <4 x float> %250, <4 x float>* %251, align 4, !tbaa !339
  %252 = add nuw nsw i64 %4, 124
  %253 = getelementptr inbounds float, float* %2, i64 %252
  %254 = getelementptr inbounds float, float* %3, i64 %252
  %255 = bitcast float* %253 to <4 x float>*
  %256 = load <4 x float>, <4 x float>* %255, align 4, !tbaa !336
  %257 = fcmp ogt <4 x float> %256, zeroinitializer
  %258 = select <4 x i1> %257, <4 x float> %256, <4 x float> zeroinitializer
  %259 = bitcast float* %254 to <4 x float>*
  store <4 x float> %258, <4 x float>* %259, align 4, !tbaa !339
  %260 = add nuw nsw i64 %4, 128
  %261 = getelementptr inbounds float, float* %2, i64 %260
  %262 = getelementptr inbounds float, float* %3, i64 %260
  %263 = bitcast float* %261 to <4 x float>*
  %264 = load <4 x float>, <4 x float>* %263, align 4, !tbaa !336
  %265 = fcmp ogt <4 x float> %264, zeroinitializer
  %266 = select <4 x i1> %265, <4 x float> %264, <4 x float> zeroinitializer
  %267 = bitcast float* %262 to <4 x float>*
  store <4 x float> %266, <4 x float>* %267, align 4, !tbaa !339
  %268 = add nuw nsw i64 %4, 132
  %269 = getelementptr inbounds float, float* %2, i64 %268
  %270 = getelementptr inbounds float, float* %3, i64 %268
  %271 = bitcast float* %269 to <4 x float>*
  %272 = load <4 x float>, <4 x float>* %271, align 4, !tbaa !336
  %273 = fcmp ogt <4 x float> %272, zeroinitializer
  %274 = select <4 x i1> %273, <4 x float> %272, <4 x float> zeroinitializer
  %275 = bitcast float* %270 to <4 x float>*
  store <4 x float> %274, <4 x float>* %275, align 4, !tbaa !339
  %276 = add nuw nsw i64 %4, 136
  %277 = getelementptr inbounds float, float* %2, i64 %276
  %278 = getelementptr inbounds float, float* %3, i64 %276
  %279 = bitcast float* %277 to <4 x float>*
  %280 = load <4 x float>, <4 x float>* %279, align 4, !tbaa !336
  %281 = fcmp ogt <4 x float> %280, zeroinitializer
  %282 = select <4 x i1> %281, <4 x float> %280, <4 x float> zeroinitializer
  %283 = bitcast float* %278 to <4 x float>*
  store <4 x float> %282, <4 x float>* %283, align 4, !tbaa !339
  %284 = add nuw nsw i64 %4, 140
  %285 = getelementptr inbounds float, float* %2, i64 %284
  %286 = getelementptr inbounds float, float* %3, i64 %284
  %287 = bitcast float* %285 to <4 x float>*
  %288 = load <4 x float>, <4 x float>* %287, align 4, !tbaa !336
  %289 = fcmp ogt <4 x float> %288, zeroinitializer
  %290 = select <4 x i1> %289, <4 x float> %288, <4 x float> zeroinitializer
  %291 = bitcast float* %286 to <4 x float>*
  store <4 x float> %290, <4 x float>* %291, align 4, !tbaa !339
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 384
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !28

for_end:                                          ; preds = %for_begin1.preheader
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
  tail call fastcc void @fused_nn_conv2d_compute_(i8* %12, i8* %16, i8* %14)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_conv2d_compute_(i8* noalias nocapture readonly, i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #3 {
entry:
  %3 = alloca [75264 x float], align 16
  %4 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar17 = phi i64 [ 0, %entry ], [ %indvar.next18, %for_end3 ]
  %5 = mul nuw nsw i64 %indvar17, 196
  %6 = mul nuw nsw i64 %indvar17, 144
  %7 = add nsw i64 %6, -13
  br label %for_begin4.preheader

for_begin7.preheader:                             ; preds = %for_end3
  %8 = bitcast i8* %1 to float*
  %9 = bitcast i8* %2 to float*
  br label %for_begin10.preheader

for_begin4.preheader:                             ; preds = %for_end6, %for_begin1.preheader
  %indvar19 = phi i64 [ 0, %for_begin1.preheader ], [ %indvar.next20, %for_end6 ]
  %10 = mul nuw nsw i64 %indvar19, 14
  %11 = add nuw nsw i64 %5, %10
  %scevgep = getelementptr [75264 x float], [75264 x float]* %3, i64 0, i64 %11
  %12 = trunc i64 %indvar19 to i32
  %13 = add i32 %12, -1
  %14 = icmp ult i32 %13, 12
  %15 = mul nuw nsw i64 %indvar19, 12
  %16 = add nsw i64 %7, %15
  br i1 %14, label %if_end.us.13, label %for_body5.preheader

for_body5.preheader:                              ; preds = %for_begin4.preheader
  %scevgep21 = bitcast float* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %scevgep21, i8 0, i64 56, i1 false)
  br label %for_end6

for_end3:                                         ; preds = %for_end6
  %indvar.next18 = add nuw nsw i64 %indvar17, 1
  %exitcond27 = icmp eq i64 %indvar.next18, 384
  br i1 %exitcond27, label %for_begin7.preheader, label %for_begin1.preheader, !prof !28

for_end6:                                         ; preds = %for_body5.preheader, %if_end.us.13
  %indvar.next20 = add nuw nsw i64 %indvar19, 1
  %exitcond26 = icmp eq i64 %indvar.next20, 14
  br i1 %exitcond26, label %for_end3, label %for_begin4.preheader, !prof !28

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %indvar = phi i64 [ 0, %for_begin7.preheader ], [ %indvar.next, %for_end12 ]
  %17 = mul nuw nsw i64 %indvar, 144
  %18 = trunc i64 %indvar to i32
  %19 = lshr i32 %18, 7
  %20 = mul nsw i32 %19, 37632
  %21 = mul nuw nsw i64 %indvar, 1728
  %22 = zext i32 %20 to i64
  br label %for_begin13.preheader

for_end9:                                         ; preds = %for_end12
  ret void

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvar10 = phi i64 [ 0, %for_begin10.preheader ], [ %26, %for_end15 ]
  %23 = mul nuw nsw i64 %indvar10, 12
  %24 = add nuw nsw i64 %23, %17
  %25 = mul nuw nsw i64 %indvar10, 14
  %26 = add nuw nsw i64 %indvar10, 1
  %27 = mul nuw nsw i64 %26, 14
  %28 = mul i64 %indvar10, 14
  %29 = add i64 %28, 28
  br label %for_body14

for_end12:                                        ; preds = %for_end15
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond13 = icmp eq i64 %indvar.next, 256
  br i1 %exitcond13, label %for_end9, label %for_begin10.preheader, !prof !28

for_body14:                                       ; preds = %for_end18, %for_begin13.preheader
  %indvars.iv7 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next8, %for_end18 ]
  %30 = add nuw nsw i64 %24, %indvars.iv7
  %31 = getelementptr inbounds float, float* %8, i64 %30
  store float 0.000000e+00, float* %31, align 4, !tbaa !342
  %32 = add nuw nsw i64 %indvars.iv7, %22
  br label %for_begin19.preheader

for_end15:                                        ; preds = %for_end18
  %exitcond12 = icmp eq i64 %26, 12
  br i1 %exitcond12, label %for_end12, label %for_begin13.preheader, !prof !28

for_begin19.preheader:                            ; preds = %for_begin19.preheader, %for_body14
  %indvars.iv = phi i64 [ 0, %for_body14 ], [ %indvars.iv.next, %for_begin19.preheader ]
  %.lcssa.lcssa3 = phi float [ 0.000000e+00, %for_body14 ], [ %98, %for_begin19.preheader ]
  %33 = mul nuw nsw i64 %indvars.iv, 196
  %34 = add nuw nsw i64 %32, %33
  %35 = mul nuw nsw i64 %indvars.iv, 9
  %36 = add nuw nsw i64 %35, %21
  %37 = add nuw nsw i64 %34, %25
  %38 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %37
  %39 = load float, float* %38, align 4, !tbaa !345
  %40 = getelementptr inbounds float, float* %9, i64 %36
  %41 = load float, float* %40, align 4, !tbaa !348
  %42 = tail call float @llvm.fmuladd.f32(float %39, float %41, float %.lcssa.lcssa3)
  %43 = add nuw nsw i64 %37, 1
  %44 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %43
  %45 = load float, float* %44, align 4, !tbaa !345
  %46 = add nuw nsw i64 %36, 1
  %47 = getelementptr inbounds float, float* %9, i64 %46
  %48 = load float, float* %47, align 4, !tbaa !348
  %49 = tail call float @llvm.fmuladd.f32(float %45, float %48, float %42)
  %50 = add nuw nsw i64 %37, 2
  %51 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %50
  %52 = load float, float* %51, align 4, !tbaa !345
  %53 = add nuw nsw i64 %36, 2
  %54 = getelementptr inbounds float, float* %9, i64 %53
  %55 = load float, float* %54, align 4, !tbaa !348
  %56 = tail call float @llvm.fmuladd.f32(float %52, float %55, float %49)
  %57 = add nuw nsw i64 %34, %27
  %58 = add nuw nsw i64 %36, 3
  %59 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %57
  %60 = load float, float* %59, align 4, !tbaa !345
  %61 = getelementptr inbounds float, float* %9, i64 %58
  %62 = load float, float* %61, align 4, !tbaa !348
  %63 = tail call float @llvm.fmuladd.f32(float %60, float %62, float %56)
  %64 = add nuw nsw i64 %57, 1
  %65 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %64
  %66 = load float, float* %65, align 4, !tbaa !345
  %67 = add nuw nsw i64 %36, 4
  %68 = getelementptr inbounds float, float* %9, i64 %67
  %69 = load float, float* %68, align 4, !tbaa !348
  %70 = tail call float @llvm.fmuladd.f32(float %66, float %69, float %63)
  %71 = add nuw nsw i64 %57, 2
  %72 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %71
  %73 = load float, float* %72, align 4, !tbaa !345
  %74 = add nuw nsw i64 %36, 5
  %75 = getelementptr inbounds float, float* %9, i64 %74
  %76 = load float, float* %75, align 4, !tbaa !348
  %77 = tail call float @llvm.fmuladd.f32(float %73, float %76, float %70)
  %78 = add nuw nsw i64 %34, %29
  %79 = add nuw nsw i64 %36, 6
  %80 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %78
  %81 = load float, float* %80, align 4, !tbaa !345
  %82 = getelementptr inbounds float, float* %9, i64 %79
  %83 = load float, float* %82, align 4, !tbaa !348
  %84 = tail call float @llvm.fmuladd.f32(float %81, float %83, float %77)
  %85 = add nuw nsw i64 %78, 1
  %86 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %85
  %87 = load float, float* %86, align 4, !tbaa !345
  %88 = add nuw nsw i64 %36, 7
  %89 = getelementptr inbounds float, float* %9, i64 %88
  %90 = load float, float* %89, align 4, !tbaa !348
  %91 = tail call float @llvm.fmuladd.f32(float %87, float %90, float %84)
  %92 = add nuw nsw i64 %78, 2
  %93 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %92
  %94 = load float, float* %93, align 4, !tbaa !345
  %95 = add nuw nsw i64 %36, 8
  %96 = getelementptr inbounds float, float* %9, i64 %95
  %97 = load float, float* %96, align 4, !tbaa !348
  %98 = tail call float @llvm.fmuladd.f32(float %94, float %97, float %91)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 192
  br i1 %exitcond, label %for_end18, label %for_begin19.preheader, !prof !28

for_end18:                                        ; preds = %for_begin19.preheader
  store float %98, float* %31, align 4, !tbaa !342
  %indvars.iv.next8 = add nuw nsw i64 %indvars.iv7, 1
  %exitcond9 = icmp eq i64 %indvars.iv.next8, 12
  br i1 %exitcond9, label %for_end15, label %for_body14, !prof !28

if_end.us.13:                                     ; preds = %for_begin4.preheader
  store float 0.000000e+00, float* %scevgep, align 8, !tbaa !345
  %99 = or i64 %11, 1
  %100 = add nsw i64 %16, 1
  %101 = getelementptr inbounds float, float* %4, i64 %100
  %102 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %99
  %103 = bitcast float* %101 to <4 x i32>*
  %104 = load <4 x i32>, <4 x i32>* %103, align 4, !tbaa !351
  %105 = bitcast float* %102 to <4 x i32>*
  store <4 x i32> %104, <4 x i32>* %105, align 4, !tbaa !345
  %106 = add nuw nsw i64 %11, 5
  %107 = add nsw i64 %16, 5
  %108 = getelementptr inbounds float, float* %4, i64 %107
  %109 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %106
  %110 = bitcast float* %108 to <4 x i32>*
  %111 = load <4 x i32>, <4 x i32>* %110, align 4, !tbaa !351
  %112 = bitcast float* %109 to <4 x i32>*
  store <4 x i32> %111, <4 x i32>* %112, align 4, !tbaa !345
  %113 = add nuw nsw i64 %11, 9
  %114 = add nsw i64 %16, 9
  %115 = getelementptr inbounds float, float* %4, i64 %114
  %116 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %113
  %117 = bitcast float* %115 to <4 x i32>*
  %118 = load <4 x i32>, <4 x i32>* %117, align 4, !tbaa !351
  %119 = bitcast float* %116 to <4 x i32>*
  store <4 x i32> %118, <4 x i32>* %119, align 4, !tbaa !345
  %120 = add nuw nsw i64 %11, 13
  %121 = getelementptr inbounds [75264 x float], [75264 x float]* %3, i64 0, i64 %120
  store float 0.000000e+00, float* %121, align 4, !tbaa !345
  br label %for_end6
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
!6 = !{!"0x2948e90.w1.b0", !7, i64 0}
!7 = !{!"0x2948e90.w2.b0", !8, i64 0}
!8 = !{!"0x2948e90.w4.b0", !9, i64 0}
!9 = !{!"0x2948e90.w8.b0", !10, i64 0}
!10 = !{!"0x2948e90.w16.b0", !11, i64 0}
!11 = !{!"0x2948e90.w32.b0", !12, i64 0}
!12 = !{!"0x2948e90.w64.b0", !13, i64 0}
!13 = !{!"0x2948e90.w128.b0", !14, i64 0}
!14 = !{!"0x2948e90.w256.b0", !15, i64 0}
!15 = !{!"0x2948e90.w512.b0", !16, i64 0}
!16 = !{!"0x2948e90.w1024.b0", !17, i64 0}
!17 = !{!"float32", !18, i64 0}
!18 = !{!"0x2948e90", !19, i64 0}
!19 = !{!"tvm-tbaa"}
!20 = !{!21, !21, i64 0}
!21 = !{!"float32", !22, i64 0}
!22 = !{!"0x2949440", !19, i64 0}
!23 = !{!24, !24, i64 0}
!24 = !{!"float32", !25, i64 0}
!25 = !{!"0x2949480", !19, i64 0}
!26 = distinct !{!26, !27}
!27 = !{!"llvm.loop.isvectorized", i32 1}
!28 = !{!"branch_weights", i32 1, i32 1048576}
!29 = !{!30, !30, i64 0}
!30 = !{!"float32", !31, i64 0}
!31 = !{!"0x2917480", !19, i64 0}
!32 = !{!33, !33, i64 0}
!33 = !{!"float32", !34, i64 0}
!34 = !{!"0x2919ec0", !19, i64 0}
!35 = !{!36, !36, i64 0}
!36 = !{!"0x2916fa0.w8.b0", !37, i64 0}
!37 = !{!"0x2916fa0.w16.b0", !38, i64 0}
!38 = !{!"0x2916fa0.w32.b0", !39, i64 0}
!39 = !{!"0x2916fa0.w64.b0", !40, i64 0}
!40 = !{!"0x2916fa0.w128.b0", !41, i64 0}
!41 = !{!"0x2916fa0.w256.b0", !42, i64 0}
!42 = !{!"0x2916fa0.w512.b0", !43, i64 0}
!43 = !{!"0x2916fa0.w1024.b0", !44, i64 0}
!44 = !{!"float32", !45, i64 0}
!45 = !{!"0x2916fa0", !19, i64 0}
!46 = !{!47, !47, i64 0}
!47 = !{!"float32", !48, i64 0}
!48 = !{!"0x2920090", !19, i64 0}
!49 = !{!50, !50, i64 0}
!50 = !{!"float32", !51, i64 0}
!51 = !{!"0x2918f40", !19, i64 0}
!52 = !{!53, !53, i64 0}
!53 = !{!"float32", !54, i64 0}
!54 = !{!"0x29172d0", !19, i64 0}
!55 = !{!56, !56, i64 0}
!56 = !{!"float32", !57, i64 0}
!57 = !{!"0x27d2020", !19, i64 0}
!58 = !{!59, !59, i64 0}
!59 = !{!"float32", !60, i64 0}
!60 = !{!"0x27d0550", !19, i64 0}
!61 = !{!62, !62, i64 0}
!62 = !{!"float32", !63, i64 0}
!63 = !{!"0x27d2290", !19, i64 0}
!64 = !{!65, !65, i64 0}
!65 = !{!"float32", !66, i64 0}
!66 = !{!"0x27d2210", !19, i64 0}
!67 = !{!68, !68, i64 0}
!68 = !{!"float32", !69, i64 0}
!69 = !{!"0x27bd230", !19, i64 0}
!70 = !{!71, !71, i64 0}
!71 = !{!"float32", !72, i64 0}
!72 = !{!"0x27bd590", !19, i64 0}
!73 = !{!74, !74, i64 0}
!74 = !{!"float32", !75, i64 0}
!75 = !{!"0x27c04e0", !19, i64 0}
!76 = !{!77, !77, i64 0}
!77 = !{!"float32", !78, i64 0}
!78 = !{!"0x27c0570", !19, i64 0}
!79 = !{!80, !80, i64 0}
!80 = !{!"float32", !81, i64 0}
!81 = !{!"0x27c9d00", !19, i64 0}
!82 = !{!83, !83, i64 0}
!83 = !{!"float32", !84, i64 0}
!84 = !{!"0x27c9cc0", !19, i64 0}
!85 = !{!86, !86, i64 0}
!86 = !{!"float32", !87, i64 0}
!87 = !{!"0x27b3f90", !19, i64 0}
!88 = !{!89, !89, i64 0}
!89 = !{!"float32", !90, i64 0}
!90 = !{!"0x27b2d30", !19, i64 0}
!91 = !{!92, !92, i64 0}
!92 = !{!"float32", !93, i64 0}
!93 = !{!"0x27b3f00", !19, i64 0}
!94 = !{!95, !95, i64 0}
!95 = !{!"float32", !96, i64 0}
!96 = !{!"0x27ab830", !19, i64 0}
!97 = !{!98, !98, i64 0}
!98 = !{!"float32", !99, i64 0}
!99 = !{!"0x27ac6e0", !19, i64 0}
!100 = !{!101, !101, i64 0}
!101 = !{!"float32", !102, i64 0}
!102 = !{!"0x27da610", !19, i64 0}
!103 = !{!104, !104, i64 0}
!104 = !{!"float32", !105, i64 0}
!105 = !{!"0x27db380", !19, i64 0}
!106 = !{!107, !107, i64 0}
!107 = !{!"float32", !108, i64 0}
!108 = !{!"0x27a2920", !19, i64 0}
!109 = !{!110, !110, i64 0}
!110 = !{!"float32", !111, i64 0}
!111 = !{!"0x27982a0", !19, i64 0}
!112 = !{!113, !113, i64 0}
!113 = !{!"float32", !114, i64 0}
!114 = !{!"0x27a2b90", !19, i64 0}
!115 = !{!116, !116, i64 0}
!116 = !{!"float32", !117, i64 0}
!117 = !{!"0x27a2b10", !19, i64 0}
!118 = !{!119, !119, i64 0}
!119 = !{!"float32", !120, i64 0}
!120 = !{!"0x27e27a0", !19, i64 0}
!121 = !{!122, !122, i64 0}
!122 = !{!"float32", !123, i64 0}
!123 = !{!"0x27e1540", !19, i64 0}
!124 = !{!125, !125, i64 0}
!125 = !{!"float32", !126, i64 0}
!126 = !{!"0x27e2710", !19, i64 0}
!127 = !{!128, !128, i64 0}
!128 = !{!"float32", !129, i64 0}
!129 = !{!"0x27857f0", !19, i64 0}
!130 = !{!131, !131, i64 0}
!131 = !{!"float32", !132, i64 0}
!132 = !{!"0x27875f0", !19, i64 0}
!133 = !{!134, !134, i64 0}
!134 = !{!"float32", !135, i64 0}
!135 = !{!"0x278e950", !19, i64 0}
!136 = !{!137, !137, i64 0}
!137 = !{!"float32", !138, i64 0}
!138 = !{!"0x2788350", !19, i64 0}
!139 = !{!140, !140, i64 0}
!140 = !{!"0x278e350.w8.b0", !141, i64 0}
!141 = !{!"0x278e350.w16.b0", !142, i64 0}
!142 = !{!"0x278e350.w32.b0", !143, i64 0}
!143 = !{!"0x278e350.w64.b0", !144, i64 0}
!144 = !{!"0x278e350.w128.b0", !145, i64 0}
!145 = !{!"0x278e350.w256.b0", !146, i64 0}
!146 = !{!"0x278e350.w512.b0", !147, i64 0}
!147 = !{!"0x278e350.w1024.b0", !148, i64 0}
!148 = !{!"float32", !149, i64 0}
!149 = !{!"0x278e350", !19, i64 0}
!150 = !{!148, !148, i64 0}
!151 = !{!152, !152, i64 0}
!152 = !{!"float32", !153, i64 0}
!153 = !{!"0x2785b20", !19, i64 0}
!154 = !{!155, !155, i64 0}
!155 = !{!"float32", !156, i64 0}
!156 = !{!"0x27724f0", !19, i64 0}
!157 = !{!158, !158, i64 0}
!158 = !{!"float32", !159, i64 0}
!159 = !{!"0x276f340", !19, i64 0}
!160 = !{!161, !161, i64 0}
!161 = !{!"float32", !162, i64 0}
!162 = !{!"0x2772580", !19, i64 0}
!163 = !{!164, !164, i64 0}
!164 = !{!"float32", !165, i64 0}
!165 = !{!"0x276f180", !19, i64 0}
!166 = !{!167, !167, i64 0}
!167 = !{!"float32", !168, i64 0}
!168 = !{!"0x2765d00", !19, i64 0}
!169 = !{!170, !170, i64 0}
!170 = !{!"float32", !171, i64 0}
!171 = !{!"0x2764aa0", !19, i64 0}
!172 = !{!173, !173, i64 0}
!173 = !{!"float32", !174, i64 0}
!174 = !{!"0x2765c70", !19, i64 0}
!175 = !{!176, !176, i64 0}
!176 = !{!"float32", !177, i64 0}
!177 = !{!"0x270d7b0", !19, i64 0}
!178 = !{!179, !179, i64 0}
!179 = !{!"float32", !180, i64 0}
!180 = !{!"0x270ddd0", !19, i64 0}
!181 = !{!182, !182, i64 0}
!182 = !{!"float32", !183, i64 0}
!183 = !{!"0x270d060", !19, i64 0}
!184 = !{!185, !185, i64 0}
!185 = !{!"float32", !186, i64 0}
!186 = !{!"0x24ee5b0", !19, i64 0}
!187 = !{!188, !188, i64 0}
!188 = !{!"float32", !189, i64 0}
!189 = !{!"0x26c4bb0", !19, i64 0}
!190 = !{!191, !191, i64 0}
!191 = !{!"float32", !192, i64 0}
!192 = !{!"0x24eccd0", !19, i64 0}
!193 = !{!194, !194, i64 0}
!194 = !{!"float32", !195, i64 0}
!195 = !{!"0x279a7e0", !19, i64 0}
!196 = !{!197, !197, i64 0}
!197 = !{!"float32", !198, i64 0}
!198 = !{!"0x279a7a0", !19, i64 0}
!199 = !{!200, !200, i64 0}
!200 = !{!"float32", !201, i64 0}
!201 = !{!"0x26bfd50", !19, i64 0}
!202 = !{!203, !203, i64 0}
!203 = !{!"float32", !204, i64 0}
!204 = !{!"0x26dea50", !19, i64 0}
!205 = !{!206, !206, i64 0}
!206 = !{!"float32", !207, i64 0}
!207 = !{!"0x26d4f40", !19, i64 0}
!208 = distinct !{!208, !27}
!209 = !{!210, !210, i64 0}
!210 = !{!"0x26debb0.w1.b0", !211, i64 0}
!211 = !{!"0x26debb0.w2.b0", !212, i64 0}
!212 = !{!"0x26debb0.w4.b0", !213, i64 0}
!213 = !{!"0x26debb0.w8.b0", !214, i64 0}
!214 = !{!"0x26debb0.w16.b0", !215, i64 0}
!215 = !{!"0x26debb0.w32.b0", !216, i64 0}
!216 = !{!"0x26debb0.w64.b0", !217, i64 0}
!217 = !{!"0x26debb0.w128.b0", !218, i64 0}
!218 = !{!"0x26debb0.w256.b0", !219, i64 0}
!219 = !{!"0x26debb0.w512.b0", !220, i64 0}
!220 = !{!"0x26debb0.w1024.b0", !221, i64 0}
!221 = !{!"float32", !222, i64 0}
!222 = !{!"0x26debb0", !19, i64 0}
!223 = !{!224, !224, i64 0}
!224 = !{!"float32", !225, i64 0}
!225 = !{!"0x26deb70", !19, i64 0}
!226 = !{!227, !227, i64 0}
!227 = !{!"float32", !228, i64 0}
!228 = !{!"0x26defa0", !19, i64 0}
!229 = distinct !{!229, !27}
!230 = !{!231, !231, i64 0}
!231 = !{!"0x277f100.w1.b0", !232, i64 0}
!232 = !{!"0x277f100.w2.b0", !233, i64 0}
!233 = !{!"0x277f100.w4.b0", !234, i64 0}
!234 = !{!"0x277f100.w8.b0", !235, i64 0}
!235 = !{!"0x277f100.w16.b0", !236, i64 0}
!236 = !{!"0x277f100.w32.b0", !237, i64 0}
!237 = !{!"0x277f100.w64.b0", !238, i64 0}
!238 = !{!"0x277f100.w128.b0", !239, i64 0}
!239 = !{!"0x277f100.w256.b0", !240, i64 0}
!240 = !{!"0x277f100.w512.b0", !241, i64 0}
!241 = !{!"0x277f100.w1024.b0", !242, i64 0}
!242 = !{!"float32", !243, i64 0}
!243 = !{!"0x277f100", !19, i64 0}
!244 = !{!245, !245, i64 0}
!245 = !{!"float32", !246, i64 0}
!246 = !{!"0x293ce40", !19, i64 0}
!247 = !{!248, !248, i64 0}
!248 = !{!"float32", !249, i64 0}
!249 = !{!"0x293ce80", !19, i64 0}
!250 = distinct !{!250, !27}
!251 = !{!252, !252, i64 0}
!252 = !{!"float32", !253, i64 0}
!253 = !{!"0x273ea10", !19, i64 0}
!254 = !{!255, !255, i64 0}
!255 = !{!"float32", !256, i64 0}
!256 = !{!"0x273fb00", !19, i64 0}
!257 = !{!258, !258, i64 0}
!258 = !{!"float32", !259, i64 0}
!259 = !{!"0x26ca380", !19, i64 0}
!260 = !{!261, !261, i64 0}
!261 = !{!"float32", !262, i64 0}
!262 = !{!"0x26ca9d0", !19, i64 0}
!263 = !{!264, !264, i64 0}
!264 = !{!"float32", !265, i64 0}
!265 = !{!"0x2702a80", !19, i64 0}
!266 = distinct !{!266, !27}
!267 = !{!268, !268, i64 0}
!268 = !{!"0x2723160.w1.b0", !269, i64 0}
!269 = !{!"0x2723160.w2.b0", !270, i64 0}
!270 = !{!"0x2723160.w4.b0", !271, i64 0}
!271 = !{!"0x2723160.w8.b0", !272, i64 0}
!272 = !{!"0x2723160.w16.b0", !273, i64 0}
!273 = !{!"0x2723160.w32.b0", !274, i64 0}
!274 = !{!"0x2723160.w64.b0", !275, i64 0}
!275 = !{!"0x2723160.w128.b0", !276, i64 0}
!276 = !{!"0x2723160.w256.b0", !277, i64 0}
!277 = !{!"0x2723160.w512.b0", !278, i64 0}
!278 = !{!"0x2723160.w1024.b0", !279, i64 0}
!279 = !{!"float32", !280, i64 0}
!280 = !{!"0x2723160", !19, i64 0}
!281 = !{!282, !282, i64 0}
!282 = !{!"float32", !283, i64 0}
!283 = !{!"0x2723120", !19, i64 0}
!284 = !{!285, !285, i64 0}
!285 = !{!"float32", !286, i64 0}
!286 = !{!"0x27235f0", !19, i64 0}
!287 = distinct !{!287, !27}
!288 = !{!289, !289, i64 0}
!289 = !{!"float32", !290, i64 0}
!290 = !{!"0x2719170", !19, i64 0}
!291 = !{!292, !292, i64 0}
!292 = !{!"float32", !293, i64 0}
!293 = !{!"0x2719790", !19, i64 0}
!294 = !{!295, !295, i64 0}
!295 = !{!"float32", !296, i64 0}
!296 = !{!"0x27173b0", !19, i64 0}
!297 = !{!298, !298, i64 0}
!298 = !{!"float32", !299, i64 0}
!299 = !{!"0x2730040", !19, i64 0}
!300 = !{!301, !301, i64 0}
!301 = !{!"float32", !302, i64 0}
!302 = !{!"0x272faf0", !19, i64 0}
!303 = !{!304, !304, i64 0}
!304 = !{!"float32", !305, i64 0}
!305 = !{!"0x26fd180", !19, i64 0}
!306 = !{!307, !307, i64 0}
!307 = !{!"float32", !308, i64 0}
!308 = !{!"0x26fcd80", !19, i64 0}
!309 = distinct !{!309, !27}
!310 = !{!311, !311, i64 0}
!311 = !{!"float32", !312, i64 0}
!312 = !{!"0x2747450", !19, i64 0}
!313 = !{!314, !314, i64 0}
!314 = !{!"float32", !315, i64 0}
!315 = !{!"0x27461f0", !19, i64 0}
!316 = !{!317, !317, i64 0}
!317 = !{!"float32", !318, i64 0}
!318 = !{!"0x27473c0", !19, i64 0}
!319 = !{!320, !320, i64 0}
!320 = !{!"float32", !321, i64 0}
!321 = !{!"0x26d9d10", !19, i64 0}
!322 = !{!323, !323, i64 0}
!323 = !{!"float32", !324, i64 0}
!324 = !{!"0x24fa7f0", !19, i64 0}
!325 = distinct !{!325, !27}
!326 = !{!327, !327, i64 0}
!327 = !{!"float32", !328, i64 0}
!328 = !{!"0x24fa600", !19, i64 0}
!329 = distinct !{!329, !27}
!330 = !{!331, !331, i64 0}
!331 = !{!"float32", !332, i64 0}
!332 = !{!"0x2738a30", !19, i64 0}
!333 = !{!334, !334, i64 0}
!334 = !{!"float32", !335, i64 0}
!335 = !{!"0x27389f0", !19, i64 0}
!336 = !{!337, !337, i64 0}
!337 = !{!"float32", !338, i64 0}
!338 = !{!"0x275d920", !19, i64 0}
!339 = !{!340, !340, i64 0}
!340 = !{!"float32", !341, i64 0}
!341 = !{!"0x275e240", !19, i64 0}
!342 = !{!343, !343, i64 0}
!343 = !{!"float32", !344, i64 0}
!344 = !{!"0x27557c0", !19, i64 0}
!345 = !{!346, !346, i64 0}
!346 = !{!"float32", !347, i64 0}
!347 = !{!"0x2752550", !19, i64 0}
!348 = !{!349, !349, i64 0}
!349 = !{!"float32", !350, i64 0}
!350 = !{!"0x2755850", !19, i64 0}
!351 = !{!352, !352, i64 0}
!352 = !{!"float32", !353, i64 0}
!353 = !{!"0x2752390", !19, i64 0}
