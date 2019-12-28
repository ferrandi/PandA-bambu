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
  %12 = load float, float* %11, align 4, !tbaa !28
  %13 = fcmp olt float %12, 0xC7EFFFFFE0000000
  %14 = select i1 %13, float 0xC7EFFFFFE0000000, float %12
  %15 = or i64 %9, 1
  %16 = getelementptr inbounds float, float* %3, i64 %15
  %17 = load float, float* %16, align 4, !tbaa !28
  %18 = fcmp ogt float %14, %17
  %19 = select i1 %18, float %14, float %17
  %20 = add nuw nsw i64 %9, 28
  %21 = getelementptr inbounds float, float* %3, i64 %20
  %22 = load float, float* %21, align 4, !tbaa !28
  %23 = fcmp ogt float %19, %22
  %24 = select i1 %23, float %19, float %22
  %25 = add nuw nsw i64 %9, 29
  %26 = getelementptr inbounds float, float* %3, i64 %25
  %27 = load float, float* %26, align 4, !tbaa !28
  %28 = fcmp ogt float %24, %27
  %29 = select i1 %28, float %24, float %27
  store float %29, float* %10, align 4, !tbaa !31
  %30 = or i64 %7, 1
  %31 = getelementptr inbounds float, float* %2, i64 %30
  %32 = or i64 %9, 2
  %33 = getelementptr inbounds float, float* %3, i64 %32
  %34 = load float, float* %33, align 4, !tbaa !28
  %35 = fcmp olt float %34, 0xC7EFFFFFE0000000
  %36 = select i1 %35, float 0xC7EFFFFFE0000000, float %34
  %37 = or i64 %9, 3
  %38 = getelementptr inbounds float, float* %3, i64 %37
  %39 = load float, float* %38, align 4, !tbaa !28
  %40 = fcmp ogt float %36, %39
  %41 = select i1 %40, float %36, float %39
  %42 = add nuw nsw i64 %32, 28
  %43 = getelementptr inbounds float, float* %3, i64 %42
  %44 = load float, float* %43, align 4, !tbaa !28
  %45 = fcmp ogt float %41, %44
  %46 = select i1 %45, float %41, float %44
  %47 = add nuw nsw i64 %32, 29
  %48 = getelementptr inbounds float, float* %3, i64 %47
  %49 = load float, float* %48, align 4, !tbaa !28
  %50 = fcmp ogt float %46, %49
  %51 = select i1 %50, float %46, float %49
  store float %51, float* %31, align 4, !tbaa !31
  %52 = add nuw nsw i64 %7, 2
  %53 = getelementptr inbounds float, float* %2, i64 %52
  %54 = or i64 %9, 4
  %55 = getelementptr inbounds float, float* %3, i64 %54
  %56 = load float, float* %55, align 4, !tbaa !28
  %57 = fcmp olt float %56, 0xC7EFFFFFE0000000
  %58 = select i1 %57, float 0xC7EFFFFFE0000000, float %56
  %59 = or i64 %9, 5
  %60 = getelementptr inbounds float, float* %3, i64 %59
  %61 = load float, float* %60, align 4, !tbaa !28
  %62 = fcmp ogt float %58, %61
  %63 = select i1 %62, float %58, float %61
  %64 = add nuw nsw i64 %54, 28
  %65 = getelementptr inbounds float, float* %3, i64 %64
  %66 = load float, float* %65, align 4, !tbaa !28
  %67 = fcmp ogt float %63, %66
  %68 = select i1 %67, float %63, float %66
  %69 = add nuw nsw i64 %54, 29
  %70 = getelementptr inbounds float, float* %3, i64 %69
  %71 = load float, float* %70, align 4, !tbaa !28
  %72 = fcmp ogt float %68, %71
  %73 = select i1 %72, float %68, float %71
  store float %73, float* %53, align 4, !tbaa !31
  %74 = add nuw nsw i64 %7, 3
  %75 = getelementptr inbounds float, float* %2, i64 %74
  %76 = or i64 %9, 6
  %77 = getelementptr inbounds float, float* %3, i64 %76
  %78 = load float, float* %77, align 4, !tbaa !28
  %79 = fcmp olt float %78, 0xC7EFFFFFE0000000
  %80 = select i1 %79, float 0xC7EFFFFFE0000000, float %78
  %81 = or i64 %9, 7
  %82 = getelementptr inbounds float, float* %3, i64 %81
  %83 = load float, float* %82, align 4, !tbaa !28
  %84 = fcmp ogt float %80, %83
  %85 = select i1 %84, float %80, float %83
  %86 = add nuw nsw i64 %76, 28
  %87 = getelementptr inbounds float, float* %3, i64 %86
  %88 = load float, float* %87, align 4, !tbaa !28
  %89 = fcmp ogt float %85, %88
  %90 = select i1 %89, float %85, float %88
  %91 = add nuw nsw i64 %76, 29
  %92 = getelementptr inbounds float, float* %3, i64 %91
  %93 = load float, float* %92, align 4, !tbaa !28
  %94 = fcmp ogt float %90, %93
  %95 = select i1 %94, float %90, float %93
  store float %95, float* %75, align 4, !tbaa !31
  %96 = add nuw nsw i64 %7, 4
  %97 = getelementptr inbounds float, float* %2, i64 %96
  %98 = add nuw nsw i64 %9, 8
  %99 = getelementptr inbounds float, float* %3, i64 %98
  %100 = load float, float* %99, align 4, !tbaa !28
  %101 = fcmp olt float %100, 0xC7EFFFFFE0000000
  %102 = select i1 %101, float 0xC7EFFFFFE0000000, float %100
  %103 = or i64 %98, 1
  %104 = getelementptr inbounds float, float* %3, i64 %103
  %105 = load float, float* %104, align 4, !tbaa !28
  %106 = fcmp ogt float %102, %105
  %107 = select i1 %106, float %102, float %105
  %108 = add nuw nsw i64 %9, 36
  %109 = getelementptr inbounds float, float* %3, i64 %108
  %110 = load float, float* %109, align 4, !tbaa !28
  %111 = fcmp ogt float %107, %110
  %112 = select i1 %111, float %107, float %110
  %113 = add nuw nsw i64 %9, 37
  %114 = getelementptr inbounds float, float* %3, i64 %113
  %115 = load float, float* %114, align 4, !tbaa !28
  %116 = fcmp ogt float %112, %115
  %117 = select i1 %116, float %112, float %115
  store float %117, float* %97, align 4, !tbaa !31
  %118 = add nuw nsw i64 %7, 5
  %119 = getelementptr inbounds float, float* %2, i64 %118
  %120 = add nuw nsw i64 %9, 10
  %121 = getelementptr inbounds float, float* %3, i64 %120
  %122 = load float, float* %121, align 4, !tbaa !28
  %123 = fcmp olt float %122, 0xC7EFFFFFE0000000
  %124 = select i1 %123, float 0xC7EFFFFFE0000000, float %122
  %125 = or i64 %120, 1
  %126 = getelementptr inbounds float, float* %3, i64 %125
  %127 = load float, float* %126, align 4, !tbaa !28
  %128 = fcmp ogt float %124, %127
  %129 = select i1 %128, float %124, float %127
  %130 = add nuw nsw i64 %9, 38
  %131 = getelementptr inbounds float, float* %3, i64 %130
  %132 = load float, float* %131, align 4, !tbaa !28
  %133 = fcmp ogt float %129, %132
  %134 = select i1 %133, float %129, float %132
  %135 = add nuw nsw i64 %9, 39
  %136 = getelementptr inbounds float, float* %3, i64 %135
  %137 = load float, float* %136, align 4, !tbaa !28
  %138 = fcmp ogt float %134, %137
  %139 = select i1 %138, float %134, float %137
  store float %139, float* %119, align 4, !tbaa !31
  %140 = add nuw nsw i64 %7, 6
  %141 = getelementptr inbounds float, float* %2, i64 %140
  %142 = add nuw nsw i64 %9, 12
  %143 = getelementptr inbounds float, float* %3, i64 %142
  %144 = load float, float* %143, align 4, !tbaa !28
  %145 = fcmp olt float %144, 0xC7EFFFFFE0000000
  %146 = select i1 %145, float 0xC7EFFFFFE0000000, float %144
  %147 = or i64 %142, 1
  %148 = getelementptr inbounds float, float* %3, i64 %147
  %149 = load float, float* %148, align 4, !tbaa !28
  %150 = fcmp ogt float %146, %149
  %151 = select i1 %150, float %146, float %149
  %152 = add nuw nsw i64 %9, 40
  %153 = getelementptr inbounds float, float* %3, i64 %152
  %154 = load float, float* %153, align 4, !tbaa !28
  %155 = fcmp ogt float %151, %154
  %156 = select i1 %155, float %151, float %154
  %157 = add nuw nsw i64 %9, 41
  %158 = getelementptr inbounds float, float* %3, i64 %157
  %159 = load float, float* %158, align 4, !tbaa !28
  %160 = fcmp ogt float %156, %159
  %161 = select i1 %160, float %156, float %159
  store float %161, float* %141, align 4, !tbaa !31
  %162 = add nuw nsw i64 %7, 7
  %163 = getelementptr inbounds float, float* %2, i64 %162
  %164 = add nuw nsw i64 %9, 14
  %165 = getelementptr inbounds float, float* %3, i64 %164
  %166 = load float, float* %165, align 4, !tbaa !28
  %167 = fcmp olt float %166, 0xC7EFFFFFE0000000
  %168 = select i1 %167, float 0xC7EFFFFFE0000000, float %166
  %169 = or i64 %164, 1
  %170 = getelementptr inbounds float, float* %3, i64 %169
  %171 = load float, float* %170, align 4, !tbaa !28
  %172 = fcmp ogt float %168, %171
  %173 = select i1 %172, float %168, float %171
  %174 = add nuw nsw i64 %9, 42
  %175 = getelementptr inbounds float, float* %3, i64 %174
  %176 = load float, float* %175, align 4, !tbaa !28
  %177 = fcmp ogt float %173, %176
  %178 = select i1 %177, float %173, float %176
  %179 = add nuw nsw i64 %9, 43
  %180 = getelementptr inbounds float, float* %3, i64 %179
  %181 = load float, float* %180, align 4, !tbaa !28
  %182 = fcmp ogt float %178, %181
  %183 = select i1 %182, float %178, float %181
  store float %183, float* %163, align 4, !tbaa !31
  %184 = add nuw nsw i64 %7, 8
  %185 = getelementptr inbounds float, float* %2, i64 %184
  %186 = add nuw nsw i64 %9, 16
  %187 = getelementptr inbounds float, float* %3, i64 %186
  %188 = load float, float* %187, align 4, !tbaa !28
  %189 = fcmp olt float %188, 0xC7EFFFFFE0000000
  %190 = select i1 %189, float 0xC7EFFFFFE0000000, float %188
  %191 = or i64 %186, 1
  %192 = getelementptr inbounds float, float* %3, i64 %191
  %193 = load float, float* %192, align 4, !tbaa !28
  %194 = fcmp ogt float %190, %193
  %195 = select i1 %194, float %190, float %193
  %196 = add nuw nsw i64 %9, 44
  %197 = getelementptr inbounds float, float* %3, i64 %196
  %198 = load float, float* %197, align 4, !tbaa !28
  %199 = fcmp ogt float %195, %198
  %200 = select i1 %199, float %195, float %198
  %201 = add nuw nsw i64 %9, 45
  %202 = getelementptr inbounds float, float* %3, i64 %201
  %203 = load float, float* %202, align 4, !tbaa !28
  %204 = fcmp ogt float %200, %203
  %205 = select i1 %204, float %200, float %203
  store float %205, float* %185, align 4, !tbaa !31
  %206 = add nuw nsw i64 %7, 9
  %207 = getelementptr inbounds float, float* %2, i64 %206
  %208 = add nuw nsw i64 %9, 18
  %209 = getelementptr inbounds float, float* %3, i64 %208
  %210 = load float, float* %209, align 4, !tbaa !28
  %211 = fcmp olt float %210, 0xC7EFFFFFE0000000
  %212 = select i1 %211, float 0xC7EFFFFFE0000000, float %210
  %213 = or i64 %208, 1
  %214 = getelementptr inbounds float, float* %3, i64 %213
  %215 = load float, float* %214, align 4, !tbaa !28
  %216 = fcmp ogt float %212, %215
  %217 = select i1 %216, float %212, float %215
  %218 = add nuw nsw i64 %9, 46
  %219 = getelementptr inbounds float, float* %3, i64 %218
  %220 = load float, float* %219, align 4, !tbaa !28
  %221 = fcmp ogt float %217, %220
  %222 = select i1 %221, float %217, float %220
  %223 = add nuw nsw i64 %9, 47
  %224 = getelementptr inbounds float, float* %3, i64 %223
  %225 = load float, float* %224, align 4, !tbaa !28
  %226 = fcmp ogt float %222, %225
  %227 = select i1 %226, float %222, float %225
  store float %227, float* %207, align 4, !tbaa !31
  %228 = add nuw nsw i64 %7, 10
  %229 = getelementptr inbounds float, float* %2, i64 %228
  %230 = add nuw nsw i64 %9, 20
  %231 = getelementptr inbounds float, float* %3, i64 %230
  %232 = load float, float* %231, align 4, !tbaa !28
  %233 = fcmp olt float %232, 0xC7EFFFFFE0000000
  %234 = select i1 %233, float 0xC7EFFFFFE0000000, float %232
  %235 = or i64 %230, 1
  %236 = getelementptr inbounds float, float* %3, i64 %235
  %237 = load float, float* %236, align 4, !tbaa !28
  %238 = fcmp ogt float %234, %237
  %239 = select i1 %238, float %234, float %237
  %240 = add nuw nsw i64 %9, 48
  %241 = getelementptr inbounds float, float* %3, i64 %240
  %242 = load float, float* %241, align 4, !tbaa !28
  %243 = fcmp ogt float %239, %242
  %244 = select i1 %243, float %239, float %242
  %245 = add nuw nsw i64 %9, 49
  %246 = getelementptr inbounds float, float* %3, i64 %245
  %247 = load float, float* %246, align 4, !tbaa !28
  %248 = fcmp ogt float %244, %247
  %249 = select i1 %248, float %244, float %247
  store float %249, float* %229, align 4, !tbaa !31
  %250 = add nuw nsw i64 %7, 11
  %251 = getelementptr inbounds float, float* %2, i64 %250
  %252 = add nuw nsw i64 %9, 22
  %253 = getelementptr inbounds float, float* %3, i64 %252
  %254 = load float, float* %253, align 4, !tbaa !28
  %255 = fcmp olt float %254, 0xC7EFFFFFE0000000
  %256 = select i1 %255, float 0xC7EFFFFFE0000000, float %254
  %257 = or i64 %252, 1
  %258 = getelementptr inbounds float, float* %3, i64 %257
  %259 = load float, float* %258, align 4, !tbaa !28
  %260 = fcmp ogt float %256, %259
  %261 = select i1 %260, float %256, float %259
  %262 = add nuw nsw i64 %9, 50
  %263 = getelementptr inbounds float, float* %3, i64 %262
  %264 = load float, float* %263, align 4, !tbaa !28
  %265 = fcmp ogt float %261, %264
  %266 = select i1 %265, float %261, float %264
  %267 = add nuw nsw i64 %9, 51
  %268 = getelementptr inbounds float, float* %3, i64 %267
  %269 = load float, float* %268, align 4, !tbaa !28
  %270 = fcmp ogt float %266, %269
  %271 = select i1 %270, float %266, float %269
  store float %271, float* %251, align 4, !tbaa !31
  %272 = add nuw nsw i64 %7, 12
  %273 = getelementptr inbounds float, float* %2, i64 %272
  %274 = add nuw nsw i64 %9, 24
  %275 = getelementptr inbounds float, float* %3, i64 %274
  %276 = load float, float* %275, align 4, !tbaa !28
  %277 = fcmp olt float %276, 0xC7EFFFFFE0000000
  %278 = select i1 %277, float 0xC7EFFFFFE0000000, float %276
  %279 = or i64 %274, 1
  %280 = getelementptr inbounds float, float* %3, i64 %279
  %281 = load float, float* %280, align 4, !tbaa !28
  %282 = fcmp ogt float %278, %281
  %283 = select i1 %282, float %278, float %281
  %284 = add nuw nsw i64 %9, 52
  %285 = getelementptr inbounds float, float* %3, i64 %284
  %286 = load float, float* %285, align 4, !tbaa !28
  %287 = fcmp ogt float %283, %286
  %288 = select i1 %287, float %283, float %286
  %289 = add nuw nsw i64 %9, 53
  %290 = getelementptr inbounds float, float* %3, i64 %289
  %291 = load float, float* %290, align 4, !tbaa !28
  %292 = fcmp ogt float %288, %291
  %293 = select i1 %292, float %288, float %291
  store float %293, float* %273, align 4, !tbaa !31
  %294 = add nuw nsw i64 %7, 13
  %295 = getelementptr inbounds float, float* %2, i64 %294
  %296 = add nuw nsw i64 %9, 26
  %297 = getelementptr inbounds float, float* %3, i64 %296
  %298 = load float, float* %297, align 4, !tbaa !28
  %299 = fcmp olt float %298, 0xC7EFFFFFE0000000
  %300 = select i1 %299, float 0xC7EFFFFFE0000000, float %298
  %301 = or i64 %296, 1
  %302 = getelementptr inbounds float, float* %3, i64 %301
  %303 = load float, float* %302, align 4, !tbaa !28
  %304 = fcmp ogt float %300, %303
  %305 = select i1 %304, float %300, float %303
  %306 = add nuw nsw i64 %9, 54
  %307 = getelementptr inbounds float, float* %3, i64 %306
  %308 = load float, float* %307, align 4, !tbaa !28
  %309 = fcmp ogt float %305, %308
  %310 = select i1 %309, float %305, float %308
  %311 = add nuw nsw i64 %9, 55
  %312 = getelementptr inbounds float, float* %3, i64 %311
  %313 = load float, float* %312, align 4, !tbaa !28
  %314 = fcmp ogt float %310, %313
  %315 = select i1 %314, float %310, float %313
  store float %315, float* %295, align 4, !tbaa !31
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 14
  br i1 %exitcond5, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  %exitcond8 = icmp eq i64 %indvars.iv.next7, 512
  br i1 %exitcond8, label %for_end, label %for_begin1.preheader, !prof !34
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

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv1 = phi i64 [ 0, %entry ], [ %indvars.iv.next2, %for_end3 ]
  %6 = mul nuw nsw i64 %indvars.iv1, 196
  %7 = getelementptr inbounds float, float* %3, i64 %indvars.iv1
  %8 = load float, float* %7, align 4, !tbaa !35
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
  %20 = load <4 x float>, <4 x float>* %19, align 4, !tbaa !38
  %21 = fadd <4 x float> %10, %20
  %22 = bitcast float* %18 to <4 x float>*
  store <4 x float> %21, <4 x float>* %22, align 4, !tbaa !41
  %23 = add nuw nsw i64 %16, 4
  %24 = getelementptr inbounds float, float* %4, i64 %23
  %25 = getelementptr inbounds float, float* %5, i64 %23
  %26 = bitcast float* %24 to <4 x float>*
  %27 = load <4 x float>, <4 x float>* %26, align 4, !tbaa !38
  %28 = fadd <4 x float> %12, %27
  %29 = bitcast float* %25 to <4 x float>*
  store <4 x float> %28, <4 x float>* %29, align 4, !tbaa !41
  %30 = add nuw nsw i64 %16, 8
  %31 = getelementptr inbounds float, float* %4, i64 %30
  %32 = getelementptr inbounds float, float* %5, i64 %30
  %33 = bitcast float* %31 to <4 x float>*
  %34 = load <4 x float>, <4 x float>* %33, align 4, !tbaa !38
  %35 = fadd <4 x float> %14, %34
  %36 = bitcast float* %32 to <4 x float>*
  store <4 x float> %35, <4 x float>* %36, align 4, !tbaa !41
  %37 = add nuw nsw i64 %16, 12
  %38 = getelementptr inbounds float, float* %4, i64 %37
  %39 = load float, float* %38, align 4, !tbaa !38
  %40 = fadd float %8, %39
  %41 = getelementptr inbounds float, float* %5, i64 %37
  store float %40, float* %41, align 4, !tbaa !41
  %42 = add nuw nsw i64 %16, 13
  %43 = getelementptr inbounds float, float* %4, i64 %42
  %44 = load float, float* %43, align 4, !tbaa !38
  %45 = fadd float %8, %44
  %46 = getelementptr inbounds float, float* %5, i64 %42
  store float %45, float* %46, align 4, !tbaa !41
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 512
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !34
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
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 100352, i1 false)
  ret void
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_max_pool2d_3(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_max_pool2d_3_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_max_pool2d_3_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %0 to float*
  %3 = bitcast i8* %1 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv6 = phi i64 [ 0, %entry ], [ %indvars.iv.next7, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv6, 3136
  %5 = mul nuw nsw i64 %indvars.iv6, 12544
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv3 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next4, %for_begin4.preheader ]
  %6 = mul nuw nsw i64 %indvars.iv3, 56
  %7 = add nuw nsw i64 %6, %4
  %8 = mul nuw nsw i64 %indvars.iv3, 224
  %9 = add nuw nsw i64 %8, %5
  %10 = getelementptr inbounds float, float* %2, i64 %7
  %11 = getelementptr inbounds float, float* %3, i64 %9
  %12 = bitcast float* %11 to <8 x float>*
  %wide.vec = load <8 x float>, <8 x float>* %12, align 4, !tbaa !44
  %strided.vec = shufflevector <8 x float> %wide.vec, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9 = shufflevector <8 x float> %wide.vec, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %13 = fcmp olt <4 x float> %strided.vec, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %14 = select <4 x i1> %13, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec
  %15 = fcmp ogt <4 x float> %14, %strided.vec9
  %16 = select <4 x i1> %15, <4 x float> %14, <4 x float> %strided.vec9
  %17 = add nuw nsw i64 %9, 112
  %18 = getelementptr inbounds float, float* %3, i64 %17
  %19 = bitcast float* %18 to <8 x float>*
  %wide.vec10 = load <8 x float>, <8 x float>* %19, align 4, !tbaa !44
  %strided.vec11 = shufflevector <8 x float> %wide.vec10, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12 = shufflevector <8 x float> %wide.vec10, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %20 = fcmp ogt <4 x float> %16, %strided.vec11
  %21 = select <4 x i1> %20, <4 x float> %16, <4 x float> %strided.vec11
  %22 = fcmp ogt <4 x float> %21, %strided.vec12
  %23 = select <4 x i1> %22, <4 x float> %21, <4 x float> %strided.vec12
  %24 = bitcast float* %10 to <4 x float>*
  store <4 x float> %23, <4 x float>* %24, align 4, !tbaa !47
  %25 = or i64 %7, 4
  %26 = getelementptr inbounds float, float* %2, i64 %25
  %27 = or i64 %9, 8
  %28 = getelementptr inbounds float, float* %3, i64 %27
  %29 = bitcast float* %28 to <8 x float>*
  %wide.vec.1 = load <8 x float>, <8 x float>* %29, align 4, !tbaa !44
  %strided.vec.1 = shufflevector <8 x float> %wide.vec.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.1 = shufflevector <8 x float> %wide.vec.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %30 = fcmp olt <4 x float> %strided.vec.1, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %31 = select <4 x i1> %30, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.1
  %32 = fcmp ogt <4 x float> %31, %strided.vec9.1
  %33 = select <4 x i1> %32, <4 x float> %31, <4 x float> %strided.vec9.1
  %34 = add nuw nsw i64 %27, 112
  %35 = getelementptr inbounds float, float* %3, i64 %34
  %36 = bitcast float* %35 to <8 x float>*
  %wide.vec10.1 = load <8 x float>, <8 x float>* %36, align 4, !tbaa !44
  %strided.vec11.1 = shufflevector <8 x float> %wide.vec10.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.1 = shufflevector <8 x float> %wide.vec10.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %37 = fcmp ogt <4 x float> %33, %strided.vec11.1
  %38 = select <4 x i1> %37, <4 x float> %33, <4 x float> %strided.vec11.1
  %39 = fcmp ogt <4 x float> %38, %strided.vec12.1
  %40 = select <4 x i1> %39, <4 x float> %38, <4 x float> %strided.vec12.1
  %41 = bitcast float* %26 to <4 x float>*
  store <4 x float> %40, <4 x float>* %41, align 4, !tbaa !47
  %42 = add nuw nsw i64 %7, 8
  %43 = getelementptr inbounds float, float* %2, i64 %42
  %44 = or i64 %9, 16
  %45 = getelementptr inbounds float, float* %3, i64 %44
  %46 = bitcast float* %45 to <8 x float>*
  %wide.vec.2 = load <8 x float>, <8 x float>* %46, align 4, !tbaa !44
  %strided.vec.2 = shufflevector <8 x float> %wide.vec.2, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.2 = shufflevector <8 x float> %wide.vec.2, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %47 = fcmp olt <4 x float> %strided.vec.2, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %48 = select <4 x i1> %47, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.2
  %49 = fcmp ogt <4 x float> %48, %strided.vec9.2
  %50 = select <4 x i1> %49, <4 x float> %48, <4 x float> %strided.vec9.2
  %51 = add nuw nsw i64 %44, 112
  %52 = getelementptr inbounds float, float* %3, i64 %51
  %53 = bitcast float* %52 to <8 x float>*
  %wide.vec10.2 = load <8 x float>, <8 x float>* %53, align 4, !tbaa !44
  %strided.vec11.2 = shufflevector <8 x float> %wide.vec10.2, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.2 = shufflevector <8 x float> %wide.vec10.2, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %54 = fcmp ogt <4 x float> %50, %strided.vec11.2
  %55 = select <4 x i1> %54, <4 x float> %50, <4 x float> %strided.vec11.2
  %56 = fcmp ogt <4 x float> %55, %strided.vec12.2
  %57 = select <4 x i1> %56, <4 x float> %55, <4 x float> %strided.vec12.2
  %58 = bitcast float* %43 to <4 x float>*
  store <4 x float> %57, <4 x float>* %58, align 4, !tbaa !47
  %59 = add nuw nsw i64 %7, 12
  %60 = getelementptr inbounds float, float* %2, i64 %59
  %61 = or i64 %9, 24
  %62 = getelementptr inbounds float, float* %3, i64 %61
  %63 = bitcast float* %62 to <8 x float>*
  %wide.vec.3 = load <8 x float>, <8 x float>* %63, align 4, !tbaa !44
  %strided.vec.3 = shufflevector <8 x float> %wide.vec.3, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.3 = shufflevector <8 x float> %wide.vec.3, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %64 = fcmp olt <4 x float> %strided.vec.3, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %65 = select <4 x i1> %64, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.3
  %66 = fcmp ogt <4 x float> %65, %strided.vec9.3
  %67 = select <4 x i1> %66, <4 x float> %65, <4 x float> %strided.vec9.3
  %68 = add nuw nsw i64 %61, 112
  %69 = getelementptr inbounds float, float* %3, i64 %68
  %70 = bitcast float* %69 to <8 x float>*
  %wide.vec10.3 = load <8 x float>, <8 x float>* %70, align 4, !tbaa !44
  %strided.vec11.3 = shufflevector <8 x float> %wide.vec10.3, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.3 = shufflevector <8 x float> %wide.vec10.3, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %71 = fcmp ogt <4 x float> %67, %strided.vec11.3
  %72 = select <4 x i1> %71, <4 x float> %67, <4 x float> %strided.vec11.3
  %73 = fcmp ogt <4 x float> %72, %strided.vec12.3
  %74 = select <4 x i1> %73, <4 x float> %72, <4 x float> %strided.vec12.3
  %75 = bitcast float* %60 to <4 x float>*
  store <4 x float> %74, <4 x float>* %75, align 4, !tbaa !47
  %76 = add nuw nsw i64 %7, 16
  %77 = getelementptr inbounds float, float* %2, i64 %76
  %78 = add nuw nsw i64 %9, 32
  %79 = getelementptr inbounds float, float* %3, i64 %78
  %80 = bitcast float* %79 to <8 x float>*
  %wide.vec.4 = load <8 x float>, <8 x float>* %80, align 4, !tbaa !44
  %strided.vec.4 = shufflevector <8 x float> %wide.vec.4, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.4 = shufflevector <8 x float> %wide.vec.4, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %81 = fcmp olt <4 x float> %strided.vec.4, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %82 = select <4 x i1> %81, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.4
  %83 = fcmp ogt <4 x float> %82, %strided.vec9.4
  %84 = select <4 x i1> %83, <4 x float> %82, <4 x float> %strided.vec9.4
  %85 = add nuw nsw i64 %9, 144
  %86 = getelementptr inbounds float, float* %3, i64 %85
  %87 = bitcast float* %86 to <8 x float>*
  %wide.vec10.4 = load <8 x float>, <8 x float>* %87, align 4, !tbaa !44
  %strided.vec11.4 = shufflevector <8 x float> %wide.vec10.4, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.4 = shufflevector <8 x float> %wide.vec10.4, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %88 = fcmp ogt <4 x float> %84, %strided.vec11.4
  %89 = select <4 x i1> %88, <4 x float> %84, <4 x float> %strided.vec11.4
  %90 = fcmp ogt <4 x float> %89, %strided.vec12.4
  %91 = select <4 x i1> %90, <4 x float> %89, <4 x float> %strided.vec12.4
  %92 = bitcast float* %77 to <4 x float>*
  store <4 x float> %91, <4 x float>* %92, align 4, !tbaa !47
  %93 = add nuw nsw i64 %7, 20
  %94 = getelementptr inbounds float, float* %2, i64 %93
  %95 = add nuw nsw i64 %9, 40
  %96 = getelementptr inbounds float, float* %3, i64 %95
  %97 = bitcast float* %96 to <8 x float>*
  %wide.vec.5 = load <8 x float>, <8 x float>* %97, align 4, !tbaa !44
  %strided.vec.5 = shufflevector <8 x float> %wide.vec.5, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.5 = shufflevector <8 x float> %wide.vec.5, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %98 = fcmp olt <4 x float> %strided.vec.5, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %99 = select <4 x i1> %98, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.5
  %100 = fcmp ogt <4 x float> %99, %strided.vec9.5
  %101 = select <4 x i1> %100, <4 x float> %99, <4 x float> %strided.vec9.5
  %102 = add nuw nsw i64 %9, 152
  %103 = getelementptr inbounds float, float* %3, i64 %102
  %104 = bitcast float* %103 to <8 x float>*
  %wide.vec10.5 = load <8 x float>, <8 x float>* %104, align 4, !tbaa !44
  %strided.vec11.5 = shufflevector <8 x float> %wide.vec10.5, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.5 = shufflevector <8 x float> %wide.vec10.5, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %105 = fcmp ogt <4 x float> %101, %strided.vec11.5
  %106 = select <4 x i1> %105, <4 x float> %101, <4 x float> %strided.vec11.5
  %107 = fcmp ogt <4 x float> %106, %strided.vec12.5
  %108 = select <4 x i1> %107, <4 x float> %106, <4 x float> %strided.vec12.5
  %109 = bitcast float* %94 to <4 x float>*
  store <4 x float> %108, <4 x float>* %109, align 4, !tbaa !47
  %110 = add nuw nsw i64 %7, 24
  %111 = getelementptr inbounds float, float* %2, i64 %110
  %112 = add nuw nsw i64 %9, 48
  %113 = getelementptr inbounds float, float* %3, i64 %112
  %114 = bitcast float* %113 to <8 x float>*
  %wide.vec.6 = load <8 x float>, <8 x float>* %114, align 4, !tbaa !44
  %strided.vec.6 = shufflevector <8 x float> %wide.vec.6, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.6 = shufflevector <8 x float> %wide.vec.6, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %115 = fcmp olt <4 x float> %strided.vec.6, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %116 = select <4 x i1> %115, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.6
  %117 = fcmp ogt <4 x float> %116, %strided.vec9.6
  %118 = select <4 x i1> %117, <4 x float> %116, <4 x float> %strided.vec9.6
  %119 = add nuw nsw i64 %9, 160
  %120 = getelementptr inbounds float, float* %3, i64 %119
  %121 = bitcast float* %120 to <8 x float>*
  %wide.vec10.6 = load <8 x float>, <8 x float>* %121, align 4, !tbaa !44
  %strided.vec11.6 = shufflevector <8 x float> %wide.vec10.6, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.6 = shufflevector <8 x float> %wide.vec10.6, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %122 = fcmp ogt <4 x float> %118, %strided.vec11.6
  %123 = select <4 x i1> %122, <4 x float> %118, <4 x float> %strided.vec11.6
  %124 = fcmp ogt <4 x float> %123, %strided.vec12.6
  %125 = select <4 x i1> %124, <4 x float> %123, <4 x float> %strided.vec12.6
  %126 = bitcast float* %111 to <4 x float>*
  store <4 x float> %125, <4 x float>* %126, align 4, !tbaa !47
  %127 = add nuw nsw i64 %7, 28
  %128 = getelementptr inbounds float, float* %2, i64 %127
  %129 = add nuw nsw i64 %9, 56
  %130 = getelementptr inbounds float, float* %3, i64 %129
  %131 = bitcast float* %130 to <8 x float>*
  %wide.vec.7 = load <8 x float>, <8 x float>* %131, align 4, !tbaa !44
  %strided.vec.7 = shufflevector <8 x float> %wide.vec.7, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.7 = shufflevector <8 x float> %wide.vec.7, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %132 = fcmp olt <4 x float> %strided.vec.7, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %133 = select <4 x i1> %132, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.7
  %134 = fcmp ogt <4 x float> %133, %strided.vec9.7
  %135 = select <4 x i1> %134, <4 x float> %133, <4 x float> %strided.vec9.7
  %136 = add nuw nsw i64 %9, 168
  %137 = getelementptr inbounds float, float* %3, i64 %136
  %138 = bitcast float* %137 to <8 x float>*
  %wide.vec10.7 = load <8 x float>, <8 x float>* %138, align 4, !tbaa !44
  %strided.vec11.7 = shufflevector <8 x float> %wide.vec10.7, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.7 = shufflevector <8 x float> %wide.vec10.7, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %139 = fcmp ogt <4 x float> %135, %strided.vec11.7
  %140 = select <4 x i1> %139, <4 x float> %135, <4 x float> %strided.vec11.7
  %141 = fcmp ogt <4 x float> %140, %strided.vec12.7
  %142 = select <4 x i1> %141, <4 x float> %140, <4 x float> %strided.vec12.7
  %143 = bitcast float* %128 to <4 x float>*
  store <4 x float> %142, <4 x float>* %143, align 4, !tbaa !47
  %144 = add nuw nsw i64 %7, 32
  %145 = getelementptr inbounds float, float* %2, i64 %144
  %146 = add nuw nsw i64 %9, 64
  %147 = getelementptr inbounds float, float* %3, i64 %146
  %148 = bitcast float* %147 to <8 x float>*
  %wide.vec.8 = load <8 x float>, <8 x float>* %148, align 4, !tbaa !44
  %strided.vec.8 = shufflevector <8 x float> %wide.vec.8, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.8 = shufflevector <8 x float> %wide.vec.8, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %149 = fcmp olt <4 x float> %strided.vec.8, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %150 = select <4 x i1> %149, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.8
  %151 = fcmp ogt <4 x float> %150, %strided.vec9.8
  %152 = select <4 x i1> %151, <4 x float> %150, <4 x float> %strided.vec9.8
  %153 = add nuw nsw i64 %9, 176
  %154 = getelementptr inbounds float, float* %3, i64 %153
  %155 = bitcast float* %154 to <8 x float>*
  %wide.vec10.8 = load <8 x float>, <8 x float>* %155, align 4, !tbaa !44
  %strided.vec11.8 = shufflevector <8 x float> %wide.vec10.8, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.8 = shufflevector <8 x float> %wide.vec10.8, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %156 = fcmp ogt <4 x float> %152, %strided.vec11.8
  %157 = select <4 x i1> %156, <4 x float> %152, <4 x float> %strided.vec11.8
  %158 = fcmp ogt <4 x float> %157, %strided.vec12.8
  %159 = select <4 x i1> %158, <4 x float> %157, <4 x float> %strided.vec12.8
  %160 = bitcast float* %145 to <4 x float>*
  store <4 x float> %159, <4 x float>* %160, align 4, !tbaa !47
  %161 = add nuw nsw i64 %7, 36
  %162 = getelementptr inbounds float, float* %2, i64 %161
  %163 = add nuw nsw i64 %9, 72
  %164 = getelementptr inbounds float, float* %3, i64 %163
  %165 = bitcast float* %164 to <8 x float>*
  %wide.vec.9 = load <8 x float>, <8 x float>* %165, align 4, !tbaa !44
  %strided.vec.9 = shufflevector <8 x float> %wide.vec.9, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.9 = shufflevector <8 x float> %wide.vec.9, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %166 = fcmp olt <4 x float> %strided.vec.9, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %167 = select <4 x i1> %166, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.9
  %168 = fcmp ogt <4 x float> %167, %strided.vec9.9
  %169 = select <4 x i1> %168, <4 x float> %167, <4 x float> %strided.vec9.9
  %170 = add nuw nsw i64 %9, 184
  %171 = getelementptr inbounds float, float* %3, i64 %170
  %172 = bitcast float* %171 to <8 x float>*
  %wide.vec10.9 = load <8 x float>, <8 x float>* %172, align 4, !tbaa !44
  %strided.vec11.9 = shufflevector <8 x float> %wide.vec10.9, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.9 = shufflevector <8 x float> %wide.vec10.9, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %173 = fcmp ogt <4 x float> %169, %strided.vec11.9
  %174 = select <4 x i1> %173, <4 x float> %169, <4 x float> %strided.vec11.9
  %175 = fcmp ogt <4 x float> %174, %strided.vec12.9
  %176 = select <4 x i1> %175, <4 x float> %174, <4 x float> %strided.vec12.9
  %177 = bitcast float* %162 to <4 x float>*
  store <4 x float> %176, <4 x float>* %177, align 4, !tbaa !47
  %178 = add nuw nsw i64 %7, 40
  %179 = getelementptr inbounds float, float* %2, i64 %178
  %180 = add nuw nsw i64 %9, 80
  %181 = getelementptr inbounds float, float* %3, i64 %180
  %182 = bitcast float* %181 to <8 x float>*
  %wide.vec.10 = load <8 x float>, <8 x float>* %182, align 4, !tbaa !44
  %strided.vec.10 = shufflevector <8 x float> %wide.vec.10, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.10 = shufflevector <8 x float> %wide.vec.10, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %183 = fcmp olt <4 x float> %strided.vec.10, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %184 = select <4 x i1> %183, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.10
  %185 = fcmp ogt <4 x float> %184, %strided.vec9.10
  %186 = select <4 x i1> %185, <4 x float> %184, <4 x float> %strided.vec9.10
  %187 = add nuw nsw i64 %9, 192
  %188 = getelementptr inbounds float, float* %3, i64 %187
  %189 = bitcast float* %188 to <8 x float>*
  %wide.vec10.10 = load <8 x float>, <8 x float>* %189, align 4, !tbaa !44
  %strided.vec11.10 = shufflevector <8 x float> %wide.vec10.10, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.10 = shufflevector <8 x float> %wide.vec10.10, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %190 = fcmp ogt <4 x float> %186, %strided.vec11.10
  %191 = select <4 x i1> %190, <4 x float> %186, <4 x float> %strided.vec11.10
  %192 = fcmp ogt <4 x float> %191, %strided.vec12.10
  %193 = select <4 x i1> %192, <4 x float> %191, <4 x float> %strided.vec12.10
  %194 = bitcast float* %179 to <4 x float>*
  store <4 x float> %193, <4 x float>* %194, align 4, !tbaa !47
  %195 = add nuw nsw i64 %7, 44
  %196 = getelementptr inbounds float, float* %2, i64 %195
  %197 = add nuw nsw i64 %9, 88
  %198 = getelementptr inbounds float, float* %3, i64 %197
  %199 = bitcast float* %198 to <8 x float>*
  %wide.vec.11 = load <8 x float>, <8 x float>* %199, align 4, !tbaa !44
  %strided.vec.11 = shufflevector <8 x float> %wide.vec.11, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.11 = shufflevector <8 x float> %wide.vec.11, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %200 = fcmp olt <4 x float> %strided.vec.11, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %201 = select <4 x i1> %200, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.11
  %202 = fcmp ogt <4 x float> %201, %strided.vec9.11
  %203 = select <4 x i1> %202, <4 x float> %201, <4 x float> %strided.vec9.11
  %204 = add nuw nsw i64 %9, 200
  %205 = getelementptr inbounds float, float* %3, i64 %204
  %206 = bitcast float* %205 to <8 x float>*
  %wide.vec10.11 = load <8 x float>, <8 x float>* %206, align 4, !tbaa !44
  %strided.vec11.11 = shufflevector <8 x float> %wide.vec10.11, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.11 = shufflevector <8 x float> %wide.vec10.11, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %207 = fcmp ogt <4 x float> %203, %strided.vec11.11
  %208 = select <4 x i1> %207, <4 x float> %203, <4 x float> %strided.vec11.11
  %209 = fcmp ogt <4 x float> %208, %strided.vec12.11
  %210 = select <4 x i1> %209, <4 x float> %208, <4 x float> %strided.vec12.11
  %211 = bitcast float* %196 to <4 x float>*
  store <4 x float> %210, <4 x float>* %211, align 4, !tbaa !47
  %212 = add nuw nsw i64 %7, 48
  %213 = getelementptr inbounds float, float* %2, i64 %212
  %214 = add nuw nsw i64 %9, 96
  %215 = getelementptr inbounds float, float* %3, i64 %214
  %216 = bitcast float* %215 to <8 x float>*
  %wide.vec.12 = load <8 x float>, <8 x float>* %216, align 4, !tbaa !44
  %strided.vec.12 = shufflevector <8 x float> %wide.vec.12, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.12 = shufflevector <8 x float> %wide.vec.12, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %217 = fcmp olt <4 x float> %strided.vec.12, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %218 = select <4 x i1> %217, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.12
  %219 = fcmp ogt <4 x float> %218, %strided.vec9.12
  %220 = select <4 x i1> %219, <4 x float> %218, <4 x float> %strided.vec9.12
  %221 = add nuw nsw i64 %9, 208
  %222 = getelementptr inbounds float, float* %3, i64 %221
  %223 = bitcast float* %222 to <8 x float>*
  %wide.vec10.12 = load <8 x float>, <8 x float>* %223, align 4, !tbaa !44
  %strided.vec11.12 = shufflevector <8 x float> %wide.vec10.12, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.12 = shufflevector <8 x float> %wide.vec10.12, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %224 = fcmp ogt <4 x float> %220, %strided.vec11.12
  %225 = select <4 x i1> %224, <4 x float> %220, <4 x float> %strided.vec11.12
  %226 = fcmp ogt <4 x float> %225, %strided.vec12.12
  %227 = select <4 x i1> %226, <4 x float> %225, <4 x float> %strided.vec12.12
  %228 = bitcast float* %213 to <4 x float>*
  store <4 x float> %227, <4 x float>* %228, align 4, !tbaa !47
  %229 = add nuw nsw i64 %7, 52
  %230 = getelementptr inbounds float, float* %2, i64 %229
  %231 = add nuw nsw i64 %9, 104
  %232 = getelementptr inbounds float, float* %3, i64 %231
  %233 = bitcast float* %232 to <8 x float>*
  %wide.vec.13 = load <8 x float>, <8 x float>* %233, align 4, !tbaa !44
  %strided.vec.13 = shufflevector <8 x float> %wide.vec.13, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.13 = shufflevector <8 x float> %wide.vec.13, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %234 = fcmp olt <4 x float> %strided.vec.13, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %235 = select <4 x i1> %234, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.13
  %236 = fcmp ogt <4 x float> %235, %strided.vec9.13
  %237 = select <4 x i1> %236, <4 x float> %235, <4 x float> %strided.vec9.13
  %238 = add nuw nsw i64 %9, 216
  %239 = getelementptr inbounds float, float* %3, i64 %238
  %240 = bitcast float* %239 to <8 x float>*
  %wide.vec10.13 = load <8 x float>, <8 x float>* %240, align 4, !tbaa !44
  %strided.vec11.13 = shufflevector <8 x float> %wide.vec10.13, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.13 = shufflevector <8 x float> %wide.vec10.13, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %241 = fcmp ogt <4 x float> %237, %strided.vec11.13
  %242 = select <4 x i1> %241, <4 x float> %237, <4 x float> %strided.vec11.13
  %243 = fcmp ogt <4 x float> %242, %strided.vec12.13
  %244 = select <4 x i1> %243, <4 x float> %242, <4 x float> %strided.vec12.13
  %245 = bitcast float* %230 to <4 x float>*
  store <4 x float> %244, <4 x float>* %245, align 4, !tbaa !47
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 56
  br i1 %exitcond5, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  %exitcond8 = icmp eq i64 %indvars.iv.next7, 128
  br i1 %exitcond8, label %for_end, label %for_begin1.preheader, !prof !34
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
  %10 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !50
  %11 = fcmp ogt <4 x float> %10, zeroinitializer
  %12 = select <4 x i1> %11, <4 x float> %10, <4 x float> zeroinitializer
  %13 = bitcast float* %8 to <4 x float>*
  store <4 x float> %12, <4 x float>* %13, align 4, !tbaa !53
  %14 = add nuw nsw i64 %6, 4
  %15 = getelementptr inbounds float, float* %2, i64 %14
  %16 = getelementptr inbounds float, float* %3, i64 %14
  %17 = bitcast float* %15 to <4 x float>*
  %18 = load <4 x float>, <4 x float>* %17, align 4, !tbaa !50
  %19 = fcmp ogt <4 x float> %18, zeroinitializer
  %20 = select <4 x i1> %19, <4 x float> %18, <4 x float> zeroinitializer
  %21 = bitcast float* %16 to <4 x float>*
  store <4 x float> %20, <4 x float>* %21, align 4, !tbaa !53
  %22 = add nuw nsw i64 %6, 8
  %23 = getelementptr inbounds float, float* %2, i64 %22
  %24 = getelementptr inbounds float, float* %3, i64 %22
  %25 = bitcast float* %23 to <4 x float>*
  %26 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !50
  %27 = fcmp ogt <4 x float> %26, zeroinitializer
  %28 = select <4 x i1> %27, <4 x float> %26, <4 x float> zeroinitializer
  %29 = bitcast float* %24 to <4 x float>*
  store <4 x float> %28, <4 x float>* %29, align 4, !tbaa !53
  %30 = add nuw nsw i64 %6, 12
  %31 = getelementptr inbounds float, float* %2, i64 %30
  %32 = load float, float* %31, align 4, !tbaa !50
  %33 = fcmp ogt float %32, 0.000000e+00
  %34 = select i1 %33, float %32, float 0.000000e+00
  %35 = getelementptr inbounds float, float* %3, i64 %30
  store float %34, float* %35, align 4, !tbaa !53
  %36 = add nuw nsw i64 %6, 13
  %37 = getelementptr inbounds float, float* %2, i64 %36
  %38 = load float, float* %37, align 4, !tbaa !50
  %39 = fcmp ogt float %38, 0.000000e+00
  %40 = select i1 %39, float %38, float 0.000000e+00
  %41 = getelementptr inbounds float, float* %3, i64 %36
  store float %40, float* %41, align 4, !tbaa !53
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 14
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 512
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !34
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
  %6 = mul nuw nsw i64 %indvars.iv3, 25088
  br label %for_body2

for_end:                                          ; preds = %for_end3
  ret void

for_body2:                                        ; preds = %for_body2, %for_begin1.preheader
  %indvars.iv = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next, %for_body2 ]
  %.02 = phi <16 x float> [ zeroinitializer, %for_begin1.preheader ], [ %15, %for_body2 ]
  %7 = shl nsw i64 %indvars.iv, 4
  %8 = getelementptr inbounds float, float* %3, i64 %7
  %9 = bitcast float* %8 to <16 x float>*
  %10 = load <16 x float>, <16 x float>* %9, align 64, !tbaa !56
  %11 = add nuw nsw i64 %7, %6
  %12 = getelementptr inbounds float, float* %4, i64 %11
  %13 = bitcast float* %12 to <16 x float>*
  %14 = load <16 x float>, <16 x float>* %13, align 64, !tbaa !59
  %15 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %10, <16 x float> %14, <16 x float> %.02)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 1568
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !34

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
  store float %32, float* %16, align 4, !tbaa !62
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 4096
  br i1 %exitcond5, label %for_end, label %for_begin1.preheader, !prof !34
}

; Function Attrs: nounwind readnone speculatable
declare <16 x float> @llvm.fmuladd.v16f32(<16 x float>, <16 x float>, <16 x float>) #4

; Function Attrs: nounwind
define dllexport i32 @fused_nn_conv2d_5(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
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
  tail call fastcc void @fused_nn_conv2d_5_compute_(i8* %12, i8* %14, i8* %16)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_conv2d_5_compute_(i8* noalias nocapture readonly, i8* noalias readonly, i8* noalias) unnamed_addr #3 {
entry:
  %3 = alloca [4 x <64 x float>], align 16
  %4 = alloca [112 x <64 x float>], align 16
  %5 = alloca [2304 x <64 x float>], align 16
  %6 = alloca [1663488 x float], align 16
  %7 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar65 = phi i64 [ 0, %entry ], [ %indvar.next66, %for_end3 ]
  %8 = mul nuw nsw i64 %indvar65, 7296
  %9 = trunc i64 %indvar65 to i32
  %10 = urem i32 %9, 114
  %11 = udiv i32 %9, 114
  %.off = add nsw i32 %10, -1
  %12 = icmp ult i32 %.off, 112
  br i1 %12, label %for_begin4.preheader.us.preheader, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep72 = getelementptr [1663488 x float], [1663488 x float]* %6, i64 0, i64 %8
  %scevgep7273 = bitcast float* %scevgep72 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep7273, i8 0, i64 29184, i1 false)
  br label %for_end3

for_begin4.preheader.us.preheader:                ; preds = %for_begin1.preheader
  %13 = mul nsw i32 %11, 802816
  %14 = add nsw i32 %13, -113
  %15 = mul nuw nsw i32 %10, 112
  %16 = add i32 %14, %15
  %17 = sext i32 %16 to i64
  br label %for_begin4.preheader.us

for_begin4.preheader.us:                          ; preds = %for_end6.us-lcssa.us.us, %for_begin4.preheader.us.preheader
  %indvars.iv77 = phi i64 [ 0, %for_begin4.preheader.us.preheader ], [ %indvars.iv.next78, %for_end6.us-lcssa.us.us ]
  %18 = mul nuw nsw i64 %indvars.iv77, 114
  %19 = add nuw nsw i64 %18, %8
  %20 = mul nuw nsw i64 %indvars.iv77, 12544
  %21 = add nsw i64 %20, %17
  br label %for_body5.us.us

for_body5.us.us:                                  ; preds = %if_end.us.us, %for_begin4.preheader.us
  %indvars.iv74 = phi i64 [ 0, %for_begin4.preheader.us ], [ %indvars.iv.next75, %if_end.us.us ]
  %22 = phi i32 [ 0, %for_begin4.preheader.us ], [ %29, %if_end.us.us ]
  %23 = add nuw nsw i64 %19, %indvars.iv74
  %trunc.us.us = trunc i32 %22 to i31
  switch i31 %trunc.us.us, label %if_then.us.us [
    i31 113, label %if_end.us.us
    i31 0, label %if_end.us.us
  ]

if_then.us.us:                                    ; preds = %for_body5.us.us
  %24 = add nsw i64 %21, %indvars.iv74
  %25 = getelementptr inbounds float, float* %7, i64 %24
  %26 = load float, float* %25, align 4, !tbaa !65
  br label %if_end.us.us

if_end.us.us:                                     ; preds = %if_then.us.us, %for_body5.us.us, %for_body5.us.us
  %27 = phi float [ %26, %if_then.us.us ], [ 0.000000e+00, %for_body5.us.us ], [ 0.000000e+00, %for_body5.us.us ]
  %28 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %23
  store float %27, float* %28, align 4, !tbaa !68
  %indvars.iv.next75 = add nuw nsw i64 %indvars.iv74, 1
  %29 = add nuw nsw i32 %22, 1
  %exitcond76 = icmp eq i64 %indvars.iv.next75, 114
  br i1 %exitcond76, label %for_end6.us-lcssa.us.us, label %for_body5.us.us, !prof !34

for_end6.us-lcssa.us.us:                          ; preds = %if_end.us.us
  %indvars.iv.next78 = add nuw nsw i64 %indvars.iv77, 1
  %exitcond79 = icmp eq i64 %indvars.iv.next78, 64
  br i1 %exitcond79, label %for_end3, label %for_begin4.preheader.us, !prof !34

for_begin7.preheader:                             ; preds = %for_end3
  %30 = bitcast [4 x <64 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [4 x <64 x float>], [4 x <64 x float>]* %3, i64 0, i64 0
  %31 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_end3:                                         ; preds = %for_end6.us-lcssa.us.us, %for_begin4.preheader.preheader
  %indvar.next66 = add nuw nsw i64 %indvar65, 1
  %exitcond80 = icmp eq i64 %indvar.next66, 228
  br i1 %exitcond80, label %for_begin7.preheader, label %for_begin1.preheader, !prof !34

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %32 = phi i32 [ 0, %for_begin7.preheader ], [ %54, %for_end12 ]
  %33 = urem i32 %32, 3
  %34 = mul nuw nsw i32 %33, 12288
  %35 = udiv i32 %32, 3
  %36 = mul nsw i32 %35, 73728
  %37 = add nuw i32 %34, %36
  %38 = mul nuw nsw i32 %33, 3
  %39 = or i32 %38, %36
  %40 = zext i32 %39 to i64
  %41 = sext i32 %37 to i64
  br label %for_begin13.preheader

for_begin19.preheader:                            ; preds = %for_end12
  %42 = getelementptr inbounds [4 x <64 x float>], [4 x <64 x float>]* %3, i64 0, i64 0, i64 64
  %43 = bitcast float* %42 to <64 x float>*
  %44 = getelementptr inbounds [4 x <64 x float>], [4 x <64 x float>]* %3, i64 0, i64 0, i64 128
  %45 = bitcast float* %44 to <64 x float>*
  %46 = getelementptr inbounds [4 x <64 x float>], [4 x <64 x float>]* %3, i64 0, i64 0, i64 192
  %47 = bitcast float* %46 to <64 x float>*
  %48 = bitcast i8* %2 to float*
  %49 = bitcast [4 x <64 x float>]* %3 to i8*
  br label %for_begin22.preheader

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvars.iv58 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next59, %for_end15 ]
  %50 = mul nuw nsw i64 %indvars.iv58, 36864
  %51 = add nuw nsw i64 %50, %41
  %52 = mul nuw nsw i64 %indvars.iv58, 576
  %53 = add nuw nsw i64 %52, %40
  br label %for_begin16.preheader

for_end12:                                        ; preds = %for_end15
  %54 = add nuw nsw i32 %32, 1
  %exitcond61 = icmp eq i32 %54, 6
  br i1 %exitcond61, label %for_begin19.preheader, label %for_begin10.preheader, !prof !34

for_begin16.preheader:                            ; preds = %for_end18, %for_begin13.preheader
  %indvars.iv55 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next56, %for_end18 ]
  %55 = shl i64 %indvars.iv55, 12
  %56 = add nuw nsw i64 %51, %55
  %57 = add nuw nsw i64 %53, %indvars.iv55
  br label %for_body17

for_end15:                                        ; preds = %for_end18
  %indvars.iv.next59 = add nuw nsw i64 %indvars.iv58, 1
  %exitcond60 = icmp eq i64 %indvars.iv.next59, 2
  br i1 %exitcond60, label %for_end12, label %for_begin13.preheader, !prof !34

for_body17:                                       ; preds = %for_body17, %for_begin16.preheader
  %indvars.iv52 = phi i64 [ 0, %for_begin16.preheader ], [ %indvars.iv.next53, %for_body17 ]
  %58 = shl i64 %indvars.iv52, 6
  %59 = add nuw nsw i64 %56, %58
  %60 = mul nuw nsw i64 %indvars.iv52, 9
  %61 = add nuw nsw i64 %57, %60
  %62 = add nuw nsw i64 %61, 1152
  %63 = add nuw nsw i64 %61, 2304
  %64 = add nuw nsw i64 %61, 3456
  %65 = add nuw nsw i64 %61, 4608
  %66 = add nuw nsw i64 %61, 5760
  %67 = add nuw nsw i64 %61, 6912
  %68 = add nuw nsw i64 %61, 8064
  %69 = add nuw nsw i64 %61, 9216
  %70 = add nuw nsw i64 %61, 10368
  %71 = add nuw nsw i64 %61, 11520
  %72 = add nuw nsw i64 %61, 12672
  %73 = add nuw nsw i64 %61, 13824
  %74 = add nuw nsw i64 %61, 14976
  %75 = add nuw nsw i64 %61, 16128
  %76 = add nuw nsw i64 %61, 17280
  %77 = add nuw nsw i64 %61, 18432
  %78 = add nuw nsw i64 %61, 19584
  %79 = add nuw nsw i64 %61, 20736
  %80 = add nuw nsw i64 %61, 21888
  %81 = add nuw nsw i64 %61, 23040
  %82 = add nuw nsw i64 %61, 24192
  %83 = add nuw nsw i64 %61, 25344
  %84 = add nuw nsw i64 %61, 26496
  %85 = add nuw nsw i64 %61, 27648
  %86 = add nuw nsw i64 %61, 28800
  %87 = add nuw nsw i64 %61, 29952
  %88 = add nuw nsw i64 %61, 31104
  %89 = add nuw nsw i64 %61, 32256
  %90 = add nuw nsw i64 %61, 33408
  %91 = add nuw nsw i64 %61, 34560
  %92 = add nuw nsw i64 %61, 35712
  %93 = add nuw nsw i64 %61, 36864
  %94 = add nuw nsw i64 %61, 38016
  %95 = add nuw nsw i64 %61, 39168
  %96 = add nuw nsw i64 %61, 40320
  %97 = add nuw nsw i64 %61, 41472
  %98 = add nuw nsw i64 %61, 42624
  %99 = add nuw nsw i64 %61, 43776
  %100 = add nuw nsw i64 %61, 44928
  %101 = add nuw nsw i64 %61, 46080
  %102 = add nuw nsw i64 %61, 47232
  %103 = add nuw nsw i64 %61, 48384
  %104 = add nuw nsw i64 %61, 49536
  %105 = add nuw nsw i64 %61, 50688
  %106 = add nuw nsw i64 %61, 51840
  %107 = add nuw nsw i64 %61, 52992
  %108 = add nuw nsw i64 %61, 54144
  %109 = add nuw nsw i64 %61, 55296
  %110 = add nuw nsw i64 %61, 56448
  %111 = add nuw nsw i64 %61, 57600
  %112 = add nuw nsw i64 %61, 58752
  %113 = add nuw nsw i64 %61, 59904
  %114 = add nuw nsw i64 %61, 61056
  %115 = add nuw nsw i64 %61, 62208
  %116 = add nuw nsw i64 %61, 63360
  %117 = add nuw nsw i64 %61, 64512
  %118 = add nuw nsw i64 %61, 65664
  %119 = add nuw nsw i64 %61, 66816
  %120 = add nuw nsw i64 %61, 67968
  %121 = add nuw nsw i64 %61, 69120
  %122 = add nuw nsw i64 %61, 70272
  %123 = add nuw nsw i64 %61, 71424
  %124 = add nuw nsw i64 %61, 72576
  %125 = getelementptr inbounds float, float* %31, i64 %61
  %126 = load float, float* %125, align 4, !tbaa !71
  %127 = insertelement <64 x float> undef, float %126, i32 0
  %128 = getelementptr inbounds float, float* %31, i64 %62
  %129 = load float, float* %128, align 4, !tbaa !71
  %130 = insertelement <64 x float> %127, float %129, i32 1
  %131 = getelementptr inbounds float, float* %31, i64 %63
  %132 = load float, float* %131, align 4, !tbaa !71
  %133 = insertelement <64 x float> %130, float %132, i32 2
  %134 = getelementptr inbounds float, float* %31, i64 %64
  %135 = load float, float* %134, align 4, !tbaa !71
  %136 = insertelement <64 x float> %133, float %135, i32 3
  %137 = getelementptr inbounds float, float* %31, i64 %65
  %138 = load float, float* %137, align 4, !tbaa !71
  %139 = insertelement <64 x float> %136, float %138, i32 4
  %140 = getelementptr inbounds float, float* %31, i64 %66
  %141 = load float, float* %140, align 4, !tbaa !71
  %142 = insertelement <64 x float> %139, float %141, i32 5
  %143 = getelementptr inbounds float, float* %31, i64 %67
  %144 = load float, float* %143, align 4, !tbaa !71
  %145 = insertelement <64 x float> %142, float %144, i32 6
  %146 = getelementptr inbounds float, float* %31, i64 %68
  %147 = load float, float* %146, align 4, !tbaa !71
  %148 = insertelement <64 x float> %145, float %147, i32 7
  %149 = getelementptr inbounds float, float* %31, i64 %69
  %150 = load float, float* %149, align 4, !tbaa !71
  %151 = insertelement <64 x float> %148, float %150, i32 8
  %152 = getelementptr inbounds float, float* %31, i64 %70
  %153 = load float, float* %152, align 4, !tbaa !71
  %154 = insertelement <64 x float> %151, float %153, i32 9
  %155 = getelementptr inbounds float, float* %31, i64 %71
  %156 = load float, float* %155, align 4, !tbaa !71
  %157 = insertelement <64 x float> %154, float %156, i32 10
  %158 = getelementptr inbounds float, float* %31, i64 %72
  %159 = load float, float* %158, align 4, !tbaa !71
  %160 = insertelement <64 x float> %157, float %159, i32 11
  %161 = getelementptr inbounds float, float* %31, i64 %73
  %162 = load float, float* %161, align 4, !tbaa !71
  %163 = insertelement <64 x float> %160, float %162, i32 12
  %164 = getelementptr inbounds float, float* %31, i64 %74
  %165 = load float, float* %164, align 4, !tbaa !71
  %166 = insertelement <64 x float> %163, float %165, i32 13
  %167 = getelementptr inbounds float, float* %31, i64 %75
  %168 = load float, float* %167, align 4, !tbaa !71
  %169 = insertelement <64 x float> %166, float %168, i32 14
  %170 = getelementptr inbounds float, float* %31, i64 %76
  %171 = load float, float* %170, align 4, !tbaa !71
  %172 = insertelement <64 x float> %169, float %171, i32 15
  %173 = getelementptr inbounds float, float* %31, i64 %77
  %174 = load float, float* %173, align 4, !tbaa !71
  %175 = insertelement <64 x float> %172, float %174, i32 16
  %176 = getelementptr inbounds float, float* %31, i64 %78
  %177 = load float, float* %176, align 4, !tbaa !71
  %178 = insertelement <64 x float> %175, float %177, i32 17
  %179 = getelementptr inbounds float, float* %31, i64 %79
  %180 = load float, float* %179, align 4, !tbaa !71
  %181 = insertelement <64 x float> %178, float %180, i32 18
  %182 = getelementptr inbounds float, float* %31, i64 %80
  %183 = load float, float* %182, align 4, !tbaa !71
  %184 = insertelement <64 x float> %181, float %183, i32 19
  %185 = getelementptr inbounds float, float* %31, i64 %81
  %186 = load float, float* %185, align 4, !tbaa !71
  %187 = insertelement <64 x float> %184, float %186, i32 20
  %188 = getelementptr inbounds float, float* %31, i64 %82
  %189 = load float, float* %188, align 4, !tbaa !71
  %190 = insertelement <64 x float> %187, float %189, i32 21
  %191 = getelementptr inbounds float, float* %31, i64 %83
  %192 = load float, float* %191, align 4, !tbaa !71
  %193 = insertelement <64 x float> %190, float %192, i32 22
  %194 = getelementptr inbounds float, float* %31, i64 %84
  %195 = load float, float* %194, align 4, !tbaa !71
  %196 = insertelement <64 x float> %193, float %195, i32 23
  %197 = getelementptr inbounds float, float* %31, i64 %85
  %198 = load float, float* %197, align 4, !tbaa !71
  %199 = insertelement <64 x float> %196, float %198, i32 24
  %200 = getelementptr inbounds float, float* %31, i64 %86
  %201 = load float, float* %200, align 4, !tbaa !71
  %202 = insertelement <64 x float> %199, float %201, i32 25
  %203 = getelementptr inbounds float, float* %31, i64 %87
  %204 = load float, float* %203, align 4, !tbaa !71
  %205 = insertelement <64 x float> %202, float %204, i32 26
  %206 = getelementptr inbounds float, float* %31, i64 %88
  %207 = load float, float* %206, align 4, !tbaa !71
  %208 = insertelement <64 x float> %205, float %207, i32 27
  %209 = getelementptr inbounds float, float* %31, i64 %89
  %210 = load float, float* %209, align 4, !tbaa !71
  %211 = insertelement <64 x float> %208, float %210, i32 28
  %212 = getelementptr inbounds float, float* %31, i64 %90
  %213 = load float, float* %212, align 4, !tbaa !71
  %214 = insertelement <64 x float> %211, float %213, i32 29
  %215 = getelementptr inbounds float, float* %31, i64 %91
  %216 = load float, float* %215, align 4, !tbaa !71
  %217 = insertelement <64 x float> %214, float %216, i32 30
  %218 = getelementptr inbounds float, float* %31, i64 %92
  %219 = load float, float* %218, align 4, !tbaa !71
  %220 = insertelement <64 x float> %217, float %219, i32 31
  %221 = getelementptr inbounds float, float* %31, i64 %93
  %222 = load float, float* %221, align 4, !tbaa !71
  %223 = insertelement <64 x float> %220, float %222, i32 32
  %224 = getelementptr inbounds float, float* %31, i64 %94
  %225 = load float, float* %224, align 4, !tbaa !71
  %226 = insertelement <64 x float> %223, float %225, i32 33
  %227 = getelementptr inbounds float, float* %31, i64 %95
  %228 = load float, float* %227, align 4, !tbaa !71
  %229 = insertelement <64 x float> %226, float %228, i32 34
  %230 = getelementptr inbounds float, float* %31, i64 %96
  %231 = load float, float* %230, align 4, !tbaa !71
  %232 = insertelement <64 x float> %229, float %231, i32 35
  %233 = getelementptr inbounds float, float* %31, i64 %97
  %234 = load float, float* %233, align 4, !tbaa !71
  %235 = insertelement <64 x float> %232, float %234, i32 36
  %236 = getelementptr inbounds float, float* %31, i64 %98
  %237 = load float, float* %236, align 4, !tbaa !71
  %238 = insertelement <64 x float> %235, float %237, i32 37
  %239 = getelementptr inbounds float, float* %31, i64 %99
  %240 = load float, float* %239, align 4, !tbaa !71
  %241 = insertelement <64 x float> %238, float %240, i32 38
  %242 = getelementptr inbounds float, float* %31, i64 %100
  %243 = load float, float* %242, align 4, !tbaa !71
  %244 = insertelement <64 x float> %241, float %243, i32 39
  %245 = getelementptr inbounds float, float* %31, i64 %101
  %246 = load float, float* %245, align 4, !tbaa !71
  %247 = insertelement <64 x float> %244, float %246, i32 40
  %248 = getelementptr inbounds float, float* %31, i64 %102
  %249 = load float, float* %248, align 4, !tbaa !71
  %250 = insertelement <64 x float> %247, float %249, i32 41
  %251 = getelementptr inbounds float, float* %31, i64 %103
  %252 = load float, float* %251, align 4, !tbaa !71
  %253 = insertelement <64 x float> %250, float %252, i32 42
  %254 = getelementptr inbounds float, float* %31, i64 %104
  %255 = load float, float* %254, align 4, !tbaa !71
  %256 = insertelement <64 x float> %253, float %255, i32 43
  %257 = getelementptr inbounds float, float* %31, i64 %105
  %258 = load float, float* %257, align 4, !tbaa !71
  %259 = insertelement <64 x float> %256, float %258, i32 44
  %260 = getelementptr inbounds float, float* %31, i64 %106
  %261 = load float, float* %260, align 4, !tbaa !71
  %262 = insertelement <64 x float> %259, float %261, i32 45
  %263 = getelementptr inbounds float, float* %31, i64 %107
  %264 = load float, float* %263, align 4, !tbaa !71
  %265 = insertelement <64 x float> %262, float %264, i32 46
  %266 = getelementptr inbounds float, float* %31, i64 %108
  %267 = load float, float* %266, align 4, !tbaa !71
  %268 = insertelement <64 x float> %265, float %267, i32 47
  %269 = getelementptr inbounds float, float* %31, i64 %109
  %270 = load float, float* %269, align 4, !tbaa !71
  %271 = insertelement <64 x float> %268, float %270, i32 48
  %272 = getelementptr inbounds float, float* %31, i64 %110
  %273 = load float, float* %272, align 4, !tbaa !71
  %274 = insertelement <64 x float> %271, float %273, i32 49
  %275 = getelementptr inbounds float, float* %31, i64 %111
  %276 = load float, float* %275, align 4, !tbaa !71
  %277 = insertelement <64 x float> %274, float %276, i32 50
  %278 = getelementptr inbounds float, float* %31, i64 %112
  %279 = load float, float* %278, align 4, !tbaa !71
  %280 = insertelement <64 x float> %277, float %279, i32 51
  %281 = getelementptr inbounds float, float* %31, i64 %113
  %282 = load float, float* %281, align 4, !tbaa !71
  %283 = insertelement <64 x float> %280, float %282, i32 52
  %284 = getelementptr inbounds float, float* %31, i64 %114
  %285 = load float, float* %284, align 4, !tbaa !71
  %286 = insertelement <64 x float> %283, float %285, i32 53
  %287 = getelementptr inbounds float, float* %31, i64 %115
  %288 = load float, float* %287, align 4, !tbaa !71
  %289 = insertelement <64 x float> %286, float %288, i32 54
  %290 = getelementptr inbounds float, float* %31, i64 %116
  %291 = load float, float* %290, align 4, !tbaa !71
  %292 = insertelement <64 x float> %289, float %291, i32 55
  %293 = getelementptr inbounds float, float* %31, i64 %117
  %294 = load float, float* %293, align 4, !tbaa !71
  %295 = insertelement <64 x float> %292, float %294, i32 56
  %296 = getelementptr inbounds float, float* %31, i64 %118
  %297 = load float, float* %296, align 4, !tbaa !71
  %298 = insertelement <64 x float> %295, float %297, i32 57
  %299 = getelementptr inbounds float, float* %31, i64 %119
  %300 = load float, float* %299, align 4, !tbaa !71
  %301 = insertelement <64 x float> %298, float %300, i32 58
  %302 = getelementptr inbounds float, float* %31, i64 %120
  %303 = load float, float* %302, align 4, !tbaa !71
  %304 = insertelement <64 x float> %301, float %303, i32 59
  %305 = getelementptr inbounds float, float* %31, i64 %121
  %306 = load float, float* %305, align 4, !tbaa !71
  %307 = insertelement <64 x float> %304, float %306, i32 60
  %308 = getelementptr inbounds float, float* %31, i64 %122
  %309 = load float, float* %308, align 4, !tbaa !71
  %310 = insertelement <64 x float> %307, float %309, i32 61
  %311 = getelementptr inbounds float, float* %31, i64 %123
  %312 = load float, float* %311, align 4, !tbaa !71
  %313 = insertelement <64 x float> %310, float %312, i32 62
  %314 = getelementptr inbounds float, float* %31, i64 %124
  %315 = load float, float* %314, align 4, !tbaa !71
  %316 = insertelement <64 x float> %313, float %315, i32 63
  %317 = getelementptr inbounds [2304 x <64 x float>], [2304 x <64 x float>]* %5, i64 0, i64 0, i64 %59
  %318 = bitcast float* %317 to <64 x float>*
  store <64 x float> %316, <64 x float>* %318, align 16, !tbaa !74
  %indvars.iv.next53 = add nuw nsw i64 %indvars.iv52, 1
  %exitcond54 = icmp eq i64 %indvars.iv.next53, 64
  br i1 %exitcond54, label %for_end18, label %for_body17, !prof !34

for_end18:                                        ; preds = %for_body17
  %indvars.iv.next56 = add nuw nsw i64 %indvars.iv55, 1
  %exitcond57 = icmp eq i64 %indvars.iv.next56, 3
  br i1 %exitcond57, label %for_end15, label %for_begin16.preheader, !prof !34

for_begin22.preheader:                            ; preds = %for_end39, %for_begin19.preheader
  %319 = phi i32 [ 0, %for_begin19.preheader ], [ %412, %for_end39 ]
  %320 = urem i32 %319, 112
  %321 = udiv i32 %319, 112
  %322 = mul nsw i32 %321, 73728
  %323 = zext i32 %322 to i64
  %reass.mul = mul nuw nsw i32 %320, 7296
  %324 = zext i32 %reass.mul to i64
  %325 = mul nuw nsw i32 %320, 7296
  %reass.mul.1 = add nuw nsw i32 %325, 7296
  %326 = zext i32 %reass.mul.1 to i64
  %327 = mul nuw nsw i32 %320, 7296
  %reass.mul.2 = add nuw nsw i32 %327, 14592
  %328 = zext i32 %reass.mul.2 to i64
  br label %for_body23

for_end21:                                        ; preds = %for_end39
  ret void

for_begin37.preheader:                            ; preds = %for_begin34.preheader
  %329 = mul nuw nsw i32 %320, 112
  %330 = mul nsw i32 %321, 802816
  %331 = or i32 %330, %329
  br label %for_begin40.preheader

for_body23:                                       ; preds = %for_begin34.preheader, %for_begin22.preheader
  %indvar = phi i64 [ 0, %for_begin22.preheader ], [ %indvar.next, %for_begin34.preheader ]
  %332 = shl i64 %indvar, 2
  %scevgep = getelementptr [112 x <64 x float>], [112 x <64 x float>]* %4, i64 0, i64 %332
  %scevgep43 = bitcast <64 x float>* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %49, i8 0, i64 1024, i1 false)
  br label %for_begin28.preheader

for_begin34.preheader:                            ; preds = %for_end33.2
  store <64 x float> %735, <64 x float>* %.sub, align 16, !tbaa !77
  store <64 x float> %736, <64 x float>* %43, align 16, !tbaa !77
  store <64 x float> %737, <64 x float>* %45, align 16, !tbaa !77
  store <64 x float> %743, <64 x float>* %47, align 16, !tbaa !77
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep43, i8* nonnull align 16 %30, i64 1024, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond44 = icmp eq i64 %indvar.next, 28
  br i1 %exitcond44, label %for_begin37.preheader, label %for_body23, !prof !34

for_begin28.preheader:                            ; preds = %for_end33.2, %for_body23
  %indvars.iv37 = phi i64 [ 0, %for_body23 ], [ %indvars.iv.next38, %for_end33.2 ]
  %.lcssa8.lcssa21 = phi <64 x float> [ zeroinitializer, %for_body23 ], [ %743, %for_end33.2 ]
  %.lcssa6.lcssa20 = phi <64 x float> [ zeroinitializer, %for_body23 ], [ %737, %for_end33.2 ]
  %.lcssa4.lcssa18 = phi <64 x float> [ zeroinitializer, %for_body23 ], [ %736, %for_end33.2 ]
  %.lcssa.lcssa16 = phi <64 x float> [ zeroinitializer, %for_body23 ], [ %735, %for_end33.2 ]
  %333 = mul nuw nsw i64 %indvars.iv37, 831744
  %334 = add nuw nsw i64 %333, %332
  %335 = mul nuw nsw i64 %indvars.iv37, 36864
  %336 = add nuw nsw i64 %335, %323
  %337 = add nsw i64 %334, %324
  %338 = trunc i64 %337 to i32
  br label %for_body32

for_body32:                                       ; preds = %for_body32, %for_begin28.preheader
  %indvars.iv = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next, %for_body32 ]
  %339 = phi <64 x float> [ %.lcssa8.lcssa21, %for_begin28.preheader ], [ %403, %for_body32 ]
  %340 = phi <64 x float> [ %.lcssa6.lcssa20, %for_begin28.preheader ], [ %397, %for_body32 ]
  %341 = phi <64 x float> [ %.lcssa4.lcssa18, %for_begin28.preheader ], [ %396, %for_body32 ]
  %342 = phi <64 x float> [ %.lcssa.lcssa16, %for_begin28.preheader ], [ %395, %for_body32 ]
  %343 = phi i32 [ 0, %for_begin28.preheader ], [ %404, %for_body32 ]
  %344 = mul nuw nsw i64 %indvars.iv, 114
  %345 = mul nuw nsw i32 %343, 114
  %346 = add nsw i64 %337, %344
  %347 = add nsw i32 %345, %338
  %348 = and i64 %346, 4294967294
  %349 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %348
  %350 = load float, float* %349, align 8, !tbaa !68
  %351 = insertelement <64 x float> undef, float %350, i32 0
  %352 = shufflevector <64 x float> %351, <64 x float> undef, <64 x i32> zeroinitializer
  %353 = shl nsw i64 %indvars.iv, 6
  %354 = add nuw nsw i64 %336, %353
  %355 = getelementptr inbounds [2304 x <64 x float>], [2304 x <64 x float>]* %5, i64 0, i64 0, i64 %354
  %356 = bitcast float* %355 to <64 x float>*
  %357 = load <64 x float>, <64 x float>* %356, align 16, !tbaa !74
  %358 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %352, <64 x float> %357, <64 x float> %342)
  %359 = or i32 %347, 1
  %360 = sext i32 %359 to i64
  %361 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %360
  %362 = load float, float* %361, align 4, !tbaa !68
  %363 = insertelement <64 x float> undef, float %362, i32 0
  %364 = shufflevector <64 x float> %363, <64 x float> undef, <64 x i32> zeroinitializer
  %365 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %364, <64 x float> %357, <64 x float> %341)
  %366 = add nuw nsw i64 %346, 2
  %367 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %366
  %368 = load float, float* %367, align 8, !tbaa !68
  %369 = insertelement <64 x float> undef, float %368, i32 0
  %370 = shufflevector <64 x float> %369, <64 x float> undef, <64 x i32> zeroinitializer
  %371 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %370, <64 x float> %357, <64 x float> %340)
  %372 = add nuw nsw i64 %346, 3
  %373 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %372
  %374 = load float, float* %373, align 4, !tbaa !68
  %375 = insertelement <64 x float> undef, float %374, i32 0
  %376 = shufflevector <64 x float> %375, <64 x float> undef, <64 x i32> zeroinitializer
  %377 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %376, <64 x float> %357, <64 x float> %339)
  %378 = add nuw nsw i64 %354, 4096
  %379 = getelementptr inbounds [2304 x <64 x float>], [2304 x <64 x float>]* %5, i64 0, i64 0, i64 %378
  %380 = bitcast float* %379 to <64 x float>*
  %381 = load <64 x float>, <64 x float>* %380, align 16, !tbaa !74
  %382 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %364, <64 x float> %381, <64 x float> %358)
  %383 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %370, <64 x float> %381, <64 x float> %365)
  %384 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %376, <64 x float> %381, <64 x float> %371)
  %385 = add nuw nsw i64 %346, 4
  %386 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %385
  %387 = load float, float* %386, align 8, !tbaa !68
  %388 = insertelement <64 x float> undef, float %387, i32 0
  %389 = shufflevector <64 x float> %388, <64 x float> undef, <64 x i32> zeroinitializer
  %390 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %389, <64 x float> %381, <64 x float> %377)
  %391 = add nuw nsw i64 %354, 8192
  %392 = getelementptr inbounds [2304 x <64 x float>], [2304 x <64 x float>]* %5, i64 0, i64 0, i64 %391
  %393 = bitcast float* %392 to <64 x float>*
  %394 = load <64 x float>, <64 x float>* %393, align 16, !tbaa !74
  %395 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %370, <64 x float> %394, <64 x float> %382)
  %396 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %376, <64 x float> %394, <64 x float> %383)
  %397 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %389, <64 x float> %394, <64 x float> %384)
  %398 = add nuw nsw i64 %346, 5
  %399 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %398
  %400 = load float, float* %399, align 4, !tbaa !68
  %401 = insertelement <64 x float> undef, float %400, i32 0
  %402 = shufflevector <64 x float> %401, <64 x float> undef, <64 x i32> zeroinitializer
  %403 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %402, <64 x float> %394, <64 x float> %390)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %404 = add nuw nsw i32 %343, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 64
  br i1 %exitcond, label %for_end33, label %for_body32, !prof !34

for_end33:                                        ; preds = %for_body32
  %405 = add nsw i64 %334, %326
  %406 = add nuw nsw i64 %336, 12288
  %407 = trunc i64 %405 to i32
  br label %for_body32.1

for_begin40.preheader:                            ; preds = %for_end42, %for_begin37.preheader
  %indvars.iv48 = phi i64 [ 0, %for_begin37.preheader ], [ %indvars.iv.next49, %for_end42 ]
  %408 = shl i64 %indvars.iv48, 8
  %indvars.iv48.tr = trunc i64 %indvars.iv48 to i32
  %409 = shl i32 %indvars.iv48.tr, 2
  %410 = add i32 %331, %409
  %411 = zext i32 %410 to i64
  br label %for_body41

for_end39:                                        ; preds = %for_end42
  %412 = add nuw nsw i32 %319, 1
  %exitcond51 = icmp eq i32 %412, 224
  br i1 %exitcond51, label %for_end21, label %for_begin22.preheader, !prof !34

for_body41:                                       ; preds = %for_body41, %for_begin40.preheader
  %indvars.iv45 = phi i64 [ 0, %for_begin40.preheader ], [ %indvars.iv.next46, %for_body41 ]
  %413 = add nuw nsw i64 %indvars.iv45, %411
  %414 = add nuw nsw i64 %413, 12544
  %415 = add nuw nsw i64 %413, 25088
  %416 = add nuw nsw i64 %413, 37632
  %417 = add nuw nsw i64 %413, 50176
  %418 = add nuw nsw i64 %413, 62720
  %419 = add nuw nsw i64 %413, 75264
  %420 = add nuw nsw i64 %413, 87808
  %421 = add nuw nsw i64 %413, 100352
  %422 = add nuw nsw i64 %413, 112896
  %423 = add nuw nsw i64 %413, 125440
  %424 = add nuw nsw i64 %413, 137984
  %425 = add nuw nsw i64 %413, 150528
  %426 = add nuw nsw i64 %413, 163072
  %427 = add nuw nsw i64 %413, 175616
  %428 = add nuw nsw i64 %413, 188160
  %429 = add nuw nsw i64 %413, 200704
  %430 = add nuw nsw i64 %413, 213248
  %431 = add nuw nsw i64 %413, 225792
  %432 = add nuw nsw i64 %413, 238336
  %433 = add nuw nsw i64 %413, 250880
  %434 = add nuw nsw i64 %413, 263424
  %435 = add nuw nsw i64 %413, 275968
  %436 = add nuw nsw i64 %413, 288512
  %437 = add nuw nsw i64 %413, 301056
  %438 = add nuw nsw i64 %413, 313600
  %439 = add nuw nsw i64 %413, 326144
  %440 = add nuw nsw i64 %413, 338688
  %441 = add nuw nsw i64 %413, 351232
  %442 = add nuw nsw i64 %413, 363776
  %443 = add nuw nsw i64 %413, 376320
  %444 = add nuw nsw i64 %413, 388864
  %445 = add nuw nsw i64 %413, 401408
  %446 = add nuw nsw i64 %413, 413952
  %447 = add nuw nsw i64 %413, 426496
  %448 = add nuw nsw i64 %413, 439040
  %449 = add nuw nsw i64 %413, 451584
  %450 = add nuw nsw i64 %413, 464128
  %451 = add nuw nsw i64 %413, 476672
  %452 = add nuw nsw i64 %413, 489216
  %453 = add nuw nsw i64 %413, 501760
  %454 = add nuw nsw i64 %413, 514304
  %455 = add nuw nsw i64 %413, 526848
  %456 = add nuw nsw i64 %413, 539392
  %457 = add nuw nsw i64 %413, 551936
  %458 = add nuw nsw i64 %413, 564480
  %459 = add nuw nsw i64 %413, 577024
  %460 = add nuw nsw i64 %413, 589568
  %461 = add nuw nsw i64 %413, 602112
  %462 = add nuw nsw i64 %413, 614656
  %463 = add nuw nsw i64 %413, 627200
  %464 = add nuw nsw i64 %413, 639744
  %465 = add nuw nsw i64 %413, 652288
  %466 = add nuw nsw i64 %413, 664832
  %467 = add nuw nsw i64 %413, 677376
  %468 = add nuw nsw i64 %413, 689920
  %469 = add nuw nsw i64 %413, 702464
  %470 = add nuw nsw i64 %413, 715008
  %471 = add nuw nsw i64 %413, 727552
  %472 = add nuw nsw i64 %413, 740096
  %473 = add nuw nsw i64 %413, 752640
  %474 = add nuw nsw i64 %413, 765184
  %475 = add nuw nsw i64 %413, 777728
  %476 = add nuw nsw i64 %413, 790272
  %477 = shl i64 %indvars.iv45, 6
  %478 = add nuw nsw i64 %477, %408
  %479 = getelementptr inbounds [112 x <64 x float>], [112 x <64 x float>]* %4, i64 0, i64 0, i64 %478
  %480 = bitcast float* %479 to <64 x float>*
  %481 = load <64 x float>, <64 x float>* %480, align 16, !tbaa !85
  %482 = getelementptr inbounds float, float* %48, i64 %413
  %483 = extractelement <64 x float> %481, i64 0
  store float %483, float* %482, align 4, !tbaa !88
  %484 = getelementptr inbounds float, float* %48, i64 %414
  %485 = extractelement <64 x float> %481, i64 1
  store float %485, float* %484, align 4, !tbaa !88
  %486 = getelementptr inbounds float, float* %48, i64 %415
  %487 = extractelement <64 x float> %481, i64 2
  store float %487, float* %486, align 4, !tbaa !88
  %488 = getelementptr inbounds float, float* %48, i64 %416
  %489 = extractelement <64 x float> %481, i64 3
  store float %489, float* %488, align 4, !tbaa !88
  %490 = getelementptr inbounds float, float* %48, i64 %417
  %491 = extractelement <64 x float> %481, i64 4
  store float %491, float* %490, align 4, !tbaa !88
  %492 = getelementptr inbounds float, float* %48, i64 %418
  %493 = extractelement <64 x float> %481, i64 5
  store float %493, float* %492, align 4, !tbaa !88
  %494 = getelementptr inbounds float, float* %48, i64 %419
  %495 = extractelement <64 x float> %481, i64 6
  store float %495, float* %494, align 4, !tbaa !88
  %496 = getelementptr inbounds float, float* %48, i64 %420
  %497 = extractelement <64 x float> %481, i64 7
  store float %497, float* %496, align 4, !tbaa !88
  %498 = getelementptr inbounds float, float* %48, i64 %421
  %499 = extractelement <64 x float> %481, i64 8
  store float %499, float* %498, align 4, !tbaa !88
  %500 = getelementptr inbounds float, float* %48, i64 %422
  %501 = extractelement <64 x float> %481, i64 9
  store float %501, float* %500, align 4, !tbaa !88
  %502 = getelementptr inbounds float, float* %48, i64 %423
  %503 = extractelement <64 x float> %481, i64 10
  store float %503, float* %502, align 4, !tbaa !88
  %504 = getelementptr inbounds float, float* %48, i64 %424
  %505 = extractelement <64 x float> %481, i64 11
  store float %505, float* %504, align 4, !tbaa !88
  %506 = getelementptr inbounds float, float* %48, i64 %425
  %507 = extractelement <64 x float> %481, i64 12
  store float %507, float* %506, align 4, !tbaa !88
  %508 = getelementptr inbounds float, float* %48, i64 %426
  %509 = extractelement <64 x float> %481, i64 13
  store float %509, float* %508, align 4, !tbaa !88
  %510 = getelementptr inbounds float, float* %48, i64 %427
  %511 = extractelement <64 x float> %481, i64 14
  store float %511, float* %510, align 4, !tbaa !88
  %512 = getelementptr inbounds float, float* %48, i64 %428
  %513 = extractelement <64 x float> %481, i64 15
  store float %513, float* %512, align 4, !tbaa !88
  %514 = getelementptr inbounds float, float* %48, i64 %429
  %515 = extractelement <64 x float> %481, i64 16
  store float %515, float* %514, align 4, !tbaa !88
  %516 = getelementptr inbounds float, float* %48, i64 %430
  %517 = extractelement <64 x float> %481, i64 17
  store float %517, float* %516, align 4, !tbaa !88
  %518 = getelementptr inbounds float, float* %48, i64 %431
  %519 = extractelement <64 x float> %481, i64 18
  store float %519, float* %518, align 4, !tbaa !88
  %520 = getelementptr inbounds float, float* %48, i64 %432
  %521 = extractelement <64 x float> %481, i64 19
  store float %521, float* %520, align 4, !tbaa !88
  %522 = getelementptr inbounds float, float* %48, i64 %433
  %523 = extractelement <64 x float> %481, i64 20
  store float %523, float* %522, align 4, !tbaa !88
  %524 = getelementptr inbounds float, float* %48, i64 %434
  %525 = extractelement <64 x float> %481, i64 21
  store float %525, float* %524, align 4, !tbaa !88
  %526 = getelementptr inbounds float, float* %48, i64 %435
  %527 = extractelement <64 x float> %481, i64 22
  store float %527, float* %526, align 4, !tbaa !88
  %528 = getelementptr inbounds float, float* %48, i64 %436
  %529 = extractelement <64 x float> %481, i64 23
  store float %529, float* %528, align 4, !tbaa !88
  %530 = getelementptr inbounds float, float* %48, i64 %437
  %531 = extractelement <64 x float> %481, i64 24
  store float %531, float* %530, align 4, !tbaa !88
  %532 = getelementptr inbounds float, float* %48, i64 %438
  %533 = extractelement <64 x float> %481, i64 25
  store float %533, float* %532, align 4, !tbaa !88
  %534 = getelementptr inbounds float, float* %48, i64 %439
  %535 = extractelement <64 x float> %481, i64 26
  store float %535, float* %534, align 4, !tbaa !88
  %536 = getelementptr inbounds float, float* %48, i64 %440
  %537 = extractelement <64 x float> %481, i64 27
  store float %537, float* %536, align 4, !tbaa !88
  %538 = getelementptr inbounds float, float* %48, i64 %441
  %539 = extractelement <64 x float> %481, i64 28
  store float %539, float* %538, align 4, !tbaa !88
  %540 = getelementptr inbounds float, float* %48, i64 %442
  %541 = extractelement <64 x float> %481, i64 29
  store float %541, float* %540, align 4, !tbaa !88
  %542 = getelementptr inbounds float, float* %48, i64 %443
  %543 = extractelement <64 x float> %481, i64 30
  store float %543, float* %542, align 4, !tbaa !88
  %544 = getelementptr inbounds float, float* %48, i64 %444
  %545 = extractelement <64 x float> %481, i64 31
  store float %545, float* %544, align 4, !tbaa !88
  %546 = getelementptr inbounds float, float* %48, i64 %445
  %547 = extractelement <64 x float> %481, i64 32
  store float %547, float* %546, align 4, !tbaa !88
  %548 = getelementptr inbounds float, float* %48, i64 %446
  %549 = extractelement <64 x float> %481, i64 33
  store float %549, float* %548, align 4, !tbaa !88
  %550 = getelementptr inbounds float, float* %48, i64 %447
  %551 = extractelement <64 x float> %481, i64 34
  store float %551, float* %550, align 4, !tbaa !88
  %552 = getelementptr inbounds float, float* %48, i64 %448
  %553 = extractelement <64 x float> %481, i64 35
  store float %553, float* %552, align 4, !tbaa !88
  %554 = getelementptr inbounds float, float* %48, i64 %449
  %555 = extractelement <64 x float> %481, i64 36
  store float %555, float* %554, align 4, !tbaa !88
  %556 = getelementptr inbounds float, float* %48, i64 %450
  %557 = extractelement <64 x float> %481, i64 37
  store float %557, float* %556, align 4, !tbaa !88
  %558 = getelementptr inbounds float, float* %48, i64 %451
  %559 = extractelement <64 x float> %481, i64 38
  store float %559, float* %558, align 4, !tbaa !88
  %560 = getelementptr inbounds float, float* %48, i64 %452
  %561 = extractelement <64 x float> %481, i64 39
  store float %561, float* %560, align 4, !tbaa !88
  %562 = getelementptr inbounds float, float* %48, i64 %453
  %563 = extractelement <64 x float> %481, i64 40
  store float %563, float* %562, align 4, !tbaa !88
  %564 = getelementptr inbounds float, float* %48, i64 %454
  %565 = extractelement <64 x float> %481, i64 41
  store float %565, float* %564, align 4, !tbaa !88
  %566 = getelementptr inbounds float, float* %48, i64 %455
  %567 = extractelement <64 x float> %481, i64 42
  store float %567, float* %566, align 4, !tbaa !88
  %568 = getelementptr inbounds float, float* %48, i64 %456
  %569 = extractelement <64 x float> %481, i64 43
  store float %569, float* %568, align 4, !tbaa !88
  %570 = getelementptr inbounds float, float* %48, i64 %457
  %571 = extractelement <64 x float> %481, i64 44
  store float %571, float* %570, align 4, !tbaa !88
  %572 = getelementptr inbounds float, float* %48, i64 %458
  %573 = extractelement <64 x float> %481, i64 45
  store float %573, float* %572, align 4, !tbaa !88
  %574 = getelementptr inbounds float, float* %48, i64 %459
  %575 = extractelement <64 x float> %481, i64 46
  store float %575, float* %574, align 4, !tbaa !88
  %576 = getelementptr inbounds float, float* %48, i64 %460
  %577 = extractelement <64 x float> %481, i64 47
  store float %577, float* %576, align 4, !tbaa !88
  %578 = getelementptr inbounds float, float* %48, i64 %461
  %579 = extractelement <64 x float> %481, i64 48
  store float %579, float* %578, align 4, !tbaa !88
  %580 = getelementptr inbounds float, float* %48, i64 %462
  %581 = extractelement <64 x float> %481, i64 49
  store float %581, float* %580, align 4, !tbaa !88
  %582 = getelementptr inbounds float, float* %48, i64 %463
  %583 = extractelement <64 x float> %481, i64 50
  store float %583, float* %582, align 4, !tbaa !88
  %584 = getelementptr inbounds float, float* %48, i64 %464
  %585 = extractelement <64 x float> %481, i64 51
  store float %585, float* %584, align 4, !tbaa !88
  %586 = getelementptr inbounds float, float* %48, i64 %465
  %587 = extractelement <64 x float> %481, i64 52
  store float %587, float* %586, align 4, !tbaa !88
  %588 = getelementptr inbounds float, float* %48, i64 %466
  %589 = extractelement <64 x float> %481, i64 53
  store float %589, float* %588, align 4, !tbaa !88
  %590 = getelementptr inbounds float, float* %48, i64 %467
  %591 = extractelement <64 x float> %481, i64 54
  store float %591, float* %590, align 4, !tbaa !88
  %592 = getelementptr inbounds float, float* %48, i64 %468
  %593 = extractelement <64 x float> %481, i64 55
  store float %593, float* %592, align 4, !tbaa !88
  %594 = getelementptr inbounds float, float* %48, i64 %469
  %595 = extractelement <64 x float> %481, i64 56
  store float %595, float* %594, align 4, !tbaa !88
  %596 = getelementptr inbounds float, float* %48, i64 %470
  %597 = extractelement <64 x float> %481, i64 57
  store float %597, float* %596, align 4, !tbaa !88
  %598 = getelementptr inbounds float, float* %48, i64 %471
  %599 = extractelement <64 x float> %481, i64 58
  store float %599, float* %598, align 4, !tbaa !88
  %600 = getelementptr inbounds float, float* %48, i64 %472
  %601 = extractelement <64 x float> %481, i64 59
  store float %601, float* %600, align 4, !tbaa !88
  %602 = getelementptr inbounds float, float* %48, i64 %473
  %603 = extractelement <64 x float> %481, i64 60
  store float %603, float* %602, align 4, !tbaa !88
  %604 = getelementptr inbounds float, float* %48, i64 %474
  %605 = extractelement <64 x float> %481, i64 61
  store float %605, float* %604, align 4, !tbaa !88
  %606 = getelementptr inbounds float, float* %48, i64 %475
  %607 = extractelement <64 x float> %481, i64 62
  store float %607, float* %606, align 4, !tbaa !88
  %608 = getelementptr inbounds float, float* %48, i64 %476
  %609 = extractelement <64 x float> %481, i64 63
  store float %609, float* %608, align 4, !tbaa !88
  %indvars.iv.next46 = add nuw nsw i64 %indvars.iv45, 1
  %exitcond47 = icmp eq i64 %indvars.iv.next46, 4
  br i1 %exitcond47, label %for_end42, label %for_body41, !prof !34

for_end42:                                        ; preds = %for_body41
  %indvars.iv.next49 = add nuw nsw i64 %indvars.iv48, 1
  %exitcond50 = icmp eq i64 %indvars.iv.next49, 28
  br i1 %exitcond50, label %for_end39, label %for_begin40.preheader, !prof !34

for_body32.1:                                     ; preds = %for_body32.1, %for_end33
  %indvars.iv.1 = phi i64 [ 0, %for_end33 ], [ %indvars.iv.next.1, %for_body32.1 ]
  %610 = phi <64 x float> [ %403, %for_end33 ], [ %674, %for_body32.1 ]
  %611 = phi <64 x float> [ %397, %for_end33 ], [ %668, %for_body32.1 ]
  %612 = phi <64 x float> [ %396, %for_end33 ], [ %667, %for_body32.1 ]
  %613 = phi <64 x float> [ %395, %for_end33 ], [ %666, %for_body32.1 ]
  %614 = phi i32 [ 0, %for_end33 ], [ %675, %for_body32.1 ]
  %615 = mul nuw nsw i64 %indvars.iv.1, 114
  %616 = mul nuw nsw i32 %614, 114
  %617 = add nsw i64 %405, %615
  %618 = add nsw i32 %616, %407
  %619 = and i64 %617, 4294967294
  %620 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %619
  %621 = load float, float* %620, align 8, !tbaa !68
  %622 = insertelement <64 x float> undef, float %621, i32 0
  %623 = shufflevector <64 x float> %622, <64 x float> undef, <64 x i32> zeroinitializer
  %624 = shl nsw i64 %indvars.iv.1, 6
  %625 = add nuw nsw i64 %406, %624
  %626 = getelementptr inbounds [2304 x <64 x float>], [2304 x <64 x float>]* %5, i64 0, i64 0, i64 %625
  %627 = bitcast float* %626 to <64 x float>*
  %628 = load <64 x float>, <64 x float>* %627, align 16, !tbaa !74
  %629 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %623, <64 x float> %628, <64 x float> %613)
  %630 = or i32 %618, 1
  %631 = sext i32 %630 to i64
  %632 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %631
  %633 = load float, float* %632, align 4, !tbaa !68
  %634 = insertelement <64 x float> undef, float %633, i32 0
  %635 = shufflevector <64 x float> %634, <64 x float> undef, <64 x i32> zeroinitializer
  %636 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %635, <64 x float> %628, <64 x float> %612)
  %637 = add nuw nsw i64 %617, 2
  %638 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %637
  %639 = load float, float* %638, align 8, !tbaa !68
  %640 = insertelement <64 x float> undef, float %639, i32 0
  %641 = shufflevector <64 x float> %640, <64 x float> undef, <64 x i32> zeroinitializer
  %642 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %641, <64 x float> %628, <64 x float> %611)
  %643 = add nuw nsw i64 %617, 3
  %644 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %643
  %645 = load float, float* %644, align 4, !tbaa !68
  %646 = insertelement <64 x float> undef, float %645, i32 0
  %647 = shufflevector <64 x float> %646, <64 x float> undef, <64 x i32> zeroinitializer
  %648 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %647, <64 x float> %628, <64 x float> %610)
  %649 = add nuw nsw i64 %625, 4096
  %650 = getelementptr inbounds [2304 x <64 x float>], [2304 x <64 x float>]* %5, i64 0, i64 0, i64 %649
  %651 = bitcast float* %650 to <64 x float>*
  %652 = load <64 x float>, <64 x float>* %651, align 16, !tbaa !74
  %653 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %635, <64 x float> %652, <64 x float> %629)
  %654 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %641, <64 x float> %652, <64 x float> %636)
  %655 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %647, <64 x float> %652, <64 x float> %642)
  %656 = add nuw nsw i64 %617, 4
  %657 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %656
  %658 = load float, float* %657, align 8, !tbaa !68
  %659 = insertelement <64 x float> undef, float %658, i32 0
  %660 = shufflevector <64 x float> %659, <64 x float> undef, <64 x i32> zeroinitializer
  %661 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %660, <64 x float> %652, <64 x float> %648)
  %662 = add nuw nsw i64 %625, 8192
  %663 = getelementptr inbounds [2304 x <64 x float>], [2304 x <64 x float>]* %5, i64 0, i64 0, i64 %662
  %664 = bitcast float* %663 to <64 x float>*
  %665 = load <64 x float>, <64 x float>* %664, align 16, !tbaa !74
  %666 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %641, <64 x float> %665, <64 x float> %653)
  %667 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %647, <64 x float> %665, <64 x float> %654)
  %668 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %660, <64 x float> %665, <64 x float> %655)
  %669 = add nuw nsw i64 %617, 5
  %670 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %669
  %671 = load float, float* %670, align 4, !tbaa !68
  %672 = insertelement <64 x float> undef, float %671, i32 0
  %673 = shufflevector <64 x float> %672, <64 x float> undef, <64 x i32> zeroinitializer
  %674 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %673, <64 x float> %665, <64 x float> %661)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %675 = add nuw nsw i32 %614, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 64
  br i1 %exitcond.1, label %for_end33.1, label %for_body32.1, !prof !34

for_end33.1:                                      ; preds = %for_body32.1
  %676 = add nsw i64 %334, %328
  %677 = add nuw nsw i64 %336, 24576
  %678 = trunc i64 %676 to i32
  br label %for_body32.2

for_body32.2:                                     ; preds = %for_body32.2, %for_end33.1
  %indvars.iv.2 = phi i64 [ 0, %for_end33.1 ], [ %indvars.iv.next.2, %for_body32.2 ]
  %679 = phi <64 x float> [ %674, %for_end33.1 ], [ %743, %for_body32.2 ]
  %680 = phi <64 x float> [ %668, %for_end33.1 ], [ %737, %for_body32.2 ]
  %681 = phi <64 x float> [ %667, %for_end33.1 ], [ %736, %for_body32.2 ]
  %682 = phi <64 x float> [ %666, %for_end33.1 ], [ %735, %for_body32.2 ]
  %683 = phi i32 [ 0, %for_end33.1 ], [ %744, %for_body32.2 ]
  %684 = mul nuw nsw i64 %indvars.iv.2, 114
  %685 = mul nuw nsw i32 %683, 114
  %686 = add nsw i64 %676, %684
  %687 = add nsw i32 %685, %678
  %688 = and i64 %686, 4294967294
  %689 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %688
  %690 = load float, float* %689, align 8, !tbaa !68
  %691 = insertelement <64 x float> undef, float %690, i32 0
  %692 = shufflevector <64 x float> %691, <64 x float> undef, <64 x i32> zeroinitializer
  %693 = shl nsw i64 %indvars.iv.2, 6
  %694 = add nuw nsw i64 %677, %693
  %695 = getelementptr inbounds [2304 x <64 x float>], [2304 x <64 x float>]* %5, i64 0, i64 0, i64 %694
  %696 = bitcast float* %695 to <64 x float>*
  %697 = load <64 x float>, <64 x float>* %696, align 16, !tbaa !74
  %698 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %692, <64 x float> %697, <64 x float> %682)
  %699 = or i32 %687, 1
  %700 = sext i32 %699 to i64
  %701 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %700
  %702 = load float, float* %701, align 4, !tbaa !68
  %703 = insertelement <64 x float> undef, float %702, i32 0
  %704 = shufflevector <64 x float> %703, <64 x float> undef, <64 x i32> zeroinitializer
  %705 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %704, <64 x float> %697, <64 x float> %681)
  %706 = add nuw nsw i64 %686, 2
  %707 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %706
  %708 = load float, float* %707, align 8, !tbaa !68
  %709 = insertelement <64 x float> undef, float %708, i32 0
  %710 = shufflevector <64 x float> %709, <64 x float> undef, <64 x i32> zeroinitializer
  %711 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %710, <64 x float> %697, <64 x float> %680)
  %712 = add nuw nsw i64 %686, 3
  %713 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %712
  %714 = load float, float* %713, align 4, !tbaa !68
  %715 = insertelement <64 x float> undef, float %714, i32 0
  %716 = shufflevector <64 x float> %715, <64 x float> undef, <64 x i32> zeroinitializer
  %717 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %716, <64 x float> %697, <64 x float> %679)
  %718 = add nuw nsw i64 %694, 4096
  %719 = getelementptr inbounds [2304 x <64 x float>], [2304 x <64 x float>]* %5, i64 0, i64 0, i64 %718
  %720 = bitcast float* %719 to <64 x float>*
  %721 = load <64 x float>, <64 x float>* %720, align 16, !tbaa !74
  %722 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %704, <64 x float> %721, <64 x float> %698)
  %723 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %710, <64 x float> %721, <64 x float> %705)
  %724 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %716, <64 x float> %721, <64 x float> %711)
  %725 = add nuw nsw i64 %686, 4
  %726 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %725
  %727 = load float, float* %726, align 8, !tbaa !68
  %728 = insertelement <64 x float> undef, float %727, i32 0
  %729 = shufflevector <64 x float> %728, <64 x float> undef, <64 x i32> zeroinitializer
  %730 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %729, <64 x float> %721, <64 x float> %717)
  %731 = add nuw nsw i64 %694, 8192
  %732 = getelementptr inbounds [2304 x <64 x float>], [2304 x <64 x float>]* %5, i64 0, i64 0, i64 %731
  %733 = bitcast float* %732 to <64 x float>*
  %734 = load <64 x float>, <64 x float>* %733, align 16, !tbaa !74
  %735 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %710, <64 x float> %734, <64 x float> %722)
  %736 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %716, <64 x float> %734, <64 x float> %723)
  %737 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %729, <64 x float> %734, <64 x float> %724)
  %738 = add nuw nsw i64 %686, 5
  %739 = getelementptr inbounds [1663488 x float], [1663488 x float]* %6, i64 0, i64 %738
  %740 = load float, float* %739, align 4, !tbaa !68
  %741 = insertelement <64 x float> undef, float %740, i32 0
  %742 = shufflevector <64 x float> %741, <64 x float> undef, <64 x i32> zeroinitializer
  %743 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %742, <64 x float> %734, <64 x float> %730)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %744 = add nuw nsw i32 %683, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 64
  br i1 %exitcond.2, label %for_end33.2, label %for_body32.2, !prof !34

for_end33.2:                                      ; preds = %for_body32.2
  %indvars.iv.next38 = add nuw nsw i64 %indvars.iv37, 1
  %exitcond39 = icmp eq i64 %indvars.iv.next38, 2
  br i1 %exitcond39, label %for_begin34.preheader, label %for_begin28.preheader, !prof !34
}

; Function Attrs: nounwind readnone speculatable
declare <64 x float> @llvm.fmuladd.v64f32(<64 x float>, <64 x float>, <64 x float>) #4

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
  %5 = load float, float* %4, align 64, !tbaa !91
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
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !105
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !105
  %11 = fmul <4 x float> %wide.load, %broadcast.splat4
  %12 = fmul <4 x float> %wide.load2, %broadcast.splat6
  %13 = getelementptr inbounds float, float* %6, i64 %index
  %14 = bitcast float* %13 to <4 x float>*
  store <4 x float> %11, <4 x float>* %14, align 4, !tbaa !108
  %15 = getelementptr inbounds float, float* %13, i64 4
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %12, <4 x float>* %16, align 4, !tbaa !108
  %index.next = add i64 %index, 8
  %17 = icmp eq i64 %index.next, 4096
  br i1 %17, label %for_end, label %vector.body, !llvm.loop !111

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
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 100352, i1 false)
  ret void
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
  %4 = mul nuw nsw i64 %indvars.iv4, 12544
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv1 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next2, %for_begin4.preheader ]
  %5 = mul nuw nsw i64 %indvars.iv1, 112
  %6 = add nuw nsw i64 %5, %4
  %7 = getelementptr inbounds float, float* %2, i64 %6
  %8 = bitcast float* %7 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !112
  %9 = fcmp ogt <4 x float> %wide.load, zeroinitializer
  %10 = select <4 x i1> %9, <4 x float> %wide.load, <4 x float> zeroinitializer
  %11 = getelementptr inbounds float, float* %3, i64 %6
  %12 = bitcast float* %11 to <4 x float>*
  store <4 x float> %10, <4 x float>* %12, align 4, !tbaa !115
  %13 = or i64 %6, 4
  %14 = getelementptr inbounds float, float* %2, i64 %13
  %15 = bitcast float* %14 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %15, align 4, !tbaa !112
  %16 = fcmp ogt <4 x float> %wide.load.1, zeroinitializer
  %17 = select <4 x i1> %16, <4 x float> %wide.load.1, <4 x float> zeroinitializer
  %18 = getelementptr inbounds float, float* %3, i64 %13
  %19 = bitcast float* %18 to <4 x float>*
  store <4 x float> %17, <4 x float>* %19, align 4, !tbaa !115
  %20 = or i64 %6, 8
  %21 = getelementptr inbounds float, float* %2, i64 %20
  %22 = bitcast float* %21 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %22, align 4, !tbaa !112
  %23 = fcmp ogt <4 x float> %wide.load.2, zeroinitializer
  %24 = select <4 x i1> %23, <4 x float> %wide.load.2, <4 x float> zeroinitializer
  %25 = getelementptr inbounds float, float* %3, i64 %20
  %26 = bitcast float* %25 to <4 x float>*
  store <4 x float> %24, <4 x float>* %26, align 4, !tbaa !115
  %27 = or i64 %6, 12
  %28 = getelementptr inbounds float, float* %2, i64 %27
  %29 = bitcast float* %28 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %29, align 4, !tbaa !112
  %30 = fcmp ogt <4 x float> %wide.load.3, zeroinitializer
  %31 = select <4 x i1> %30, <4 x float> %wide.load.3, <4 x float> zeroinitializer
  %32 = getelementptr inbounds float, float* %3, i64 %27
  %33 = bitcast float* %32 to <4 x float>*
  store <4 x float> %31, <4 x float>* %33, align 4, !tbaa !115
  %34 = add nuw nsw i64 %6, 16
  %35 = getelementptr inbounds float, float* %2, i64 %34
  %36 = bitcast float* %35 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %36, align 4, !tbaa !112
  %37 = fcmp ogt <4 x float> %wide.load.4, zeroinitializer
  %38 = select <4 x i1> %37, <4 x float> %wide.load.4, <4 x float> zeroinitializer
  %39 = getelementptr inbounds float, float* %3, i64 %34
  %40 = bitcast float* %39 to <4 x float>*
  store <4 x float> %38, <4 x float>* %40, align 4, !tbaa !115
  %41 = add nuw nsw i64 %6, 20
  %42 = getelementptr inbounds float, float* %2, i64 %41
  %43 = bitcast float* %42 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %43, align 4, !tbaa !112
  %44 = fcmp ogt <4 x float> %wide.load.5, zeroinitializer
  %45 = select <4 x i1> %44, <4 x float> %wide.load.5, <4 x float> zeroinitializer
  %46 = getelementptr inbounds float, float* %3, i64 %41
  %47 = bitcast float* %46 to <4 x float>*
  store <4 x float> %45, <4 x float>* %47, align 4, !tbaa !115
  %48 = add nuw nsw i64 %6, 24
  %49 = getelementptr inbounds float, float* %2, i64 %48
  %50 = bitcast float* %49 to <4 x float>*
  %wide.load.6 = load <4 x float>, <4 x float>* %50, align 4, !tbaa !112
  %51 = fcmp ogt <4 x float> %wide.load.6, zeroinitializer
  %52 = select <4 x i1> %51, <4 x float> %wide.load.6, <4 x float> zeroinitializer
  %53 = getelementptr inbounds float, float* %3, i64 %48
  %54 = bitcast float* %53 to <4 x float>*
  store <4 x float> %52, <4 x float>* %54, align 4, !tbaa !115
  %55 = add nuw nsw i64 %6, 28
  %56 = getelementptr inbounds float, float* %2, i64 %55
  %57 = bitcast float* %56 to <4 x float>*
  %wide.load.7 = load <4 x float>, <4 x float>* %57, align 4, !tbaa !112
  %58 = fcmp ogt <4 x float> %wide.load.7, zeroinitializer
  %59 = select <4 x i1> %58, <4 x float> %wide.load.7, <4 x float> zeroinitializer
  %60 = getelementptr inbounds float, float* %3, i64 %55
  %61 = bitcast float* %60 to <4 x float>*
  store <4 x float> %59, <4 x float>* %61, align 4, !tbaa !115
  %62 = add nuw nsw i64 %6, 32
  %63 = getelementptr inbounds float, float* %2, i64 %62
  %64 = bitcast float* %63 to <4 x float>*
  %wide.load.8 = load <4 x float>, <4 x float>* %64, align 4, !tbaa !112
  %65 = fcmp ogt <4 x float> %wide.load.8, zeroinitializer
  %66 = select <4 x i1> %65, <4 x float> %wide.load.8, <4 x float> zeroinitializer
  %67 = getelementptr inbounds float, float* %3, i64 %62
  %68 = bitcast float* %67 to <4 x float>*
  store <4 x float> %66, <4 x float>* %68, align 4, !tbaa !115
  %69 = add nuw nsw i64 %6, 36
  %70 = getelementptr inbounds float, float* %2, i64 %69
  %71 = bitcast float* %70 to <4 x float>*
  %wide.load.9 = load <4 x float>, <4 x float>* %71, align 4, !tbaa !112
  %72 = fcmp ogt <4 x float> %wide.load.9, zeroinitializer
  %73 = select <4 x i1> %72, <4 x float> %wide.load.9, <4 x float> zeroinitializer
  %74 = getelementptr inbounds float, float* %3, i64 %69
  %75 = bitcast float* %74 to <4 x float>*
  store <4 x float> %73, <4 x float>* %75, align 4, !tbaa !115
  %76 = add nuw nsw i64 %6, 40
  %77 = getelementptr inbounds float, float* %2, i64 %76
  %78 = bitcast float* %77 to <4 x float>*
  %wide.load.10 = load <4 x float>, <4 x float>* %78, align 4, !tbaa !112
  %79 = fcmp ogt <4 x float> %wide.load.10, zeroinitializer
  %80 = select <4 x i1> %79, <4 x float> %wide.load.10, <4 x float> zeroinitializer
  %81 = getelementptr inbounds float, float* %3, i64 %76
  %82 = bitcast float* %81 to <4 x float>*
  store <4 x float> %80, <4 x float>* %82, align 4, !tbaa !115
  %83 = add nuw nsw i64 %6, 44
  %84 = getelementptr inbounds float, float* %2, i64 %83
  %85 = bitcast float* %84 to <4 x float>*
  %wide.load.11 = load <4 x float>, <4 x float>* %85, align 4, !tbaa !112
  %86 = fcmp ogt <4 x float> %wide.load.11, zeroinitializer
  %87 = select <4 x i1> %86, <4 x float> %wide.load.11, <4 x float> zeroinitializer
  %88 = getelementptr inbounds float, float* %3, i64 %83
  %89 = bitcast float* %88 to <4 x float>*
  store <4 x float> %87, <4 x float>* %89, align 4, !tbaa !115
  %90 = add nuw nsw i64 %6, 48
  %91 = getelementptr inbounds float, float* %2, i64 %90
  %92 = bitcast float* %91 to <4 x float>*
  %wide.load.12 = load <4 x float>, <4 x float>* %92, align 4, !tbaa !112
  %93 = fcmp ogt <4 x float> %wide.load.12, zeroinitializer
  %94 = select <4 x i1> %93, <4 x float> %wide.load.12, <4 x float> zeroinitializer
  %95 = getelementptr inbounds float, float* %3, i64 %90
  %96 = bitcast float* %95 to <4 x float>*
  store <4 x float> %94, <4 x float>* %96, align 4, !tbaa !115
  %97 = add nuw nsw i64 %6, 52
  %98 = getelementptr inbounds float, float* %2, i64 %97
  %99 = bitcast float* %98 to <4 x float>*
  %wide.load.13 = load <4 x float>, <4 x float>* %99, align 4, !tbaa !112
  %100 = fcmp ogt <4 x float> %wide.load.13, zeroinitializer
  %101 = select <4 x i1> %100, <4 x float> %wide.load.13, <4 x float> zeroinitializer
  %102 = getelementptr inbounds float, float* %3, i64 %97
  %103 = bitcast float* %102 to <4 x float>*
  store <4 x float> %101, <4 x float>* %103, align 4, !tbaa !115
  %104 = add nuw nsw i64 %6, 56
  %105 = getelementptr inbounds float, float* %2, i64 %104
  %106 = bitcast float* %105 to <4 x float>*
  %wide.load.14 = load <4 x float>, <4 x float>* %106, align 4, !tbaa !112
  %107 = fcmp ogt <4 x float> %wide.load.14, zeroinitializer
  %108 = select <4 x i1> %107, <4 x float> %wide.load.14, <4 x float> zeroinitializer
  %109 = getelementptr inbounds float, float* %3, i64 %104
  %110 = bitcast float* %109 to <4 x float>*
  store <4 x float> %108, <4 x float>* %110, align 4, !tbaa !115
  %111 = add nuw nsw i64 %6, 60
  %112 = getelementptr inbounds float, float* %2, i64 %111
  %113 = bitcast float* %112 to <4 x float>*
  %wide.load.15 = load <4 x float>, <4 x float>* %113, align 4, !tbaa !112
  %114 = fcmp ogt <4 x float> %wide.load.15, zeroinitializer
  %115 = select <4 x i1> %114, <4 x float> %wide.load.15, <4 x float> zeroinitializer
  %116 = getelementptr inbounds float, float* %3, i64 %111
  %117 = bitcast float* %116 to <4 x float>*
  store <4 x float> %115, <4 x float>* %117, align 4, !tbaa !115
  %118 = add nuw nsw i64 %6, 64
  %119 = getelementptr inbounds float, float* %2, i64 %118
  %120 = bitcast float* %119 to <4 x float>*
  %wide.load.16 = load <4 x float>, <4 x float>* %120, align 4, !tbaa !112
  %121 = fcmp ogt <4 x float> %wide.load.16, zeroinitializer
  %122 = select <4 x i1> %121, <4 x float> %wide.load.16, <4 x float> zeroinitializer
  %123 = getelementptr inbounds float, float* %3, i64 %118
  %124 = bitcast float* %123 to <4 x float>*
  store <4 x float> %122, <4 x float>* %124, align 4, !tbaa !115
  %125 = add nuw nsw i64 %6, 68
  %126 = getelementptr inbounds float, float* %2, i64 %125
  %127 = bitcast float* %126 to <4 x float>*
  %wide.load.17 = load <4 x float>, <4 x float>* %127, align 4, !tbaa !112
  %128 = fcmp ogt <4 x float> %wide.load.17, zeroinitializer
  %129 = select <4 x i1> %128, <4 x float> %wide.load.17, <4 x float> zeroinitializer
  %130 = getelementptr inbounds float, float* %3, i64 %125
  %131 = bitcast float* %130 to <4 x float>*
  store <4 x float> %129, <4 x float>* %131, align 4, !tbaa !115
  %132 = add nuw nsw i64 %6, 72
  %133 = getelementptr inbounds float, float* %2, i64 %132
  %134 = bitcast float* %133 to <4 x float>*
  %wide.load.18 = load <4 x float>, <4 x float>* %134, align 4, !tbaa !112
  %135 = fcmp ogt <4 x float> %wide.load.18, zeroinitializer
  %136 = select <4 x i1> %135, <4 x float> %wide.load.18, <4 x float> zeroinitializer
  %137 = getelementptr inbounds float, float* %3, i64 %132
  %138 = bitcast float* %137 to <4 x float>*
  store <4 x float> %136, <4 x float>* %138, align 4, !tbaa !115
  %139 = add nuw nsw i64 %6, 76
  %140 = getelementptr inbounds float, float* %2, i64 %139
  %141 = bitcast float* %140 to <4 x float>*
  %wide.load.19 = load <4 x float>, <4 x float>* %141, align 4, !tbaa !112
  %142 = fcmp ogt <4 x float> %wide.load.19, zeroinitializer
  %143 = select <4 x i1> %142, <4 x float> %wide.load.19, <4 x float> zeroinitializer
  %144 = getelementptr inbounds float, float* %3, i64 %139
  %145 = bitcast float* %144 to <4 x float>*
  store <4 x float> %143, <4 x float>* %145, align 4, !tbaa !115
  %146 = add nuw nsw i64 %6, 80
  %147 = getelementptr inbounds float, float* %2, i64 %146
  %148 = bitcast float* %147 to <4 x float>*
  %wide.load.20 = load <4 x float>, <4 x float>* %148, align 4, !tbaa !112
  %149 = fcmp ogt <4 x float> %wide.load.20, zeroinitializer
  %150 = select <4 x i1> %149, <4 x float> %wide.load.20, <4 x float> zeroinitializer
  %151 = getelementptr inbounds float, float* %3, i64 %146
  %152 = bitcast float* %151 to <4 x float>*
  store <4 x float> %150, <4 x float>* %152, align 4, !tbaa !115
  %153 = add nuw nsw i64 %6, 84
  %154 = getelementptr inbounds float, float* %2, i64 %153
  %155 = bitcast float* %154 to <4 x float>*
  %wide.load.21 = load <4 x float>, <4 x float>* %155, align 4, !tbaa !112
  %156 = fcmp ogt <4 x float> %wide.load.21, zeroinitializer
  %157 = select <4 x i1> %156, <4 x float> %wide.load.21, <4 x float> zeroinitializer
  %158 = getelementptr inbounds float, float* %3, i64 %153
  %159 = bitcast float* %158 to <4 x float>*
  store <4 x float> %157, <4 x float>* %159, align 4, !tbaa !115
  %160 = add nuw nsw i64 %6, 88
  %161 = getelementptr inbounds float, float* %2, i64 %160
  %162 = bitcast float* %161 to <4 x float>*
  %wide.load.22 = load <4 x float>, <4 x float>* %162, align 4, !tbaa !112
  %163 = fcmp ogt <4 x float> %wide.load.22, zeroinitializer
  %164 = select <4 x i1> %163, <4 x float> %wide.load.22, <4 x float> zeroinitializer
  %165 = getelementptr inbounds float, float* %3, i64 %160
  %166 = bitcast float* %165 to <4 x float>*
  store <4 x float> %164, <4 x float>* %166, align 4, !tbaa !115
  %167 = add nuw nsw i64 %6, 92
  %168 = getelementptr inbounds float, float* %2, i64 %167
  %169 = bitcast float* %168 to <4 x float>*
  %wide.load.23 = load <4 x float>, <4 x float>* %169, align 4, !tbaa !112
  %170 = fcmp ogt <4 x float> %wide.load.23, zeroinitializer
  %171 = select <4 x i1> %170, <4 x float> %wide.load.23, <4 x float> zeroinitializer
  %172 = getelementptr inbounds float, float* %3, i64 %167
  %173 = bitcast float* %172 to <4 x float>*
  store <4 x float> %171, <4 x float>* %173, align 4, !tbaa !115
  %174 = add nuw nsw i64 %6, 96
  %175 = getelementptr inbounds float, float* %2, i64 %174
  %176 = bitcast float* %175 to <4 x float>*
  %wide.load.24 = load <4 x float>, <4 x float>* %176, align 4, !tbaa !112
  %177 = fcmp ogt <4 x float> %wide.load.24, zeroinitializer
  %178 = select <4 x i1> %177, <4 x float> %wide.load.24, <4 x float> zeroinitializer
  %179 = getelementptr inbounds float, float* %3, i64 %174
  %180 = bitcast float* %179 to <4 x float>*
  store <4 x float> %178, <4 x float>* %180, align 4, !tbaa !115
  %181 = add nuw nsw i64 %6, 100
  %182 = getelementptr inbounds float, float* %2, i64 %181
  %183 = bitcast float* %182 to <4 x float>*
  %wide.load.25 = load <4 x float>, <4 x float>* %183, align 4, !tbaa !112
  %184 = fcmp ogt <4 x float> %wide.load.25, zeroinitializer
  %185 = select <4 x i1> %184, <4 x float> %wide.load.25, <4 x float> zeroinitializer
  %186 = getelementptr inbounds float, float* %3, i64 %181
  %187 = bitcast float* %186 to <4 x float>*
  store <4 x float> %185, <4 x float>* %187, align 4, !tbaa !115
  %188 = add nuw nsw i64 %6, 104
  %189 = getelementptr inbounds float, float* %2, i64 %188
  %190 = bitcast float* %189 to <4 x float>*
  %wide.load.26 = load <4 x float>, <4 x float>* %190, align 4, !tbaa !112
  %191 = fcmp ogt <4 x float> %wide.load.26, zeroinitializer
  %192 = select <4 x i1> %191, <4 x float> %wide.load.26, <4 x float> zeroinitializer
  %193 = getelementptr inbounds float, float* %3, i64 %188
  %194 = bitcast float* %193 to <4 x float>*
  store <4 x float> %192, <4 x float>* %194, align 4, !tbaa !115
  %195 = add nuw nsw i64 %6, 108
  %196 = getelementptr inbounds float, float* %2, i64 %195
  %197 = bitcast float* %196 to <4 x float>*
  %wide.load.27 = load <4 x float>, <4 x float>* %197, align 4, !tbaa !112
  %198 = fcmp ogt <4 x float> %wide.load.27, zeroinitializer
  %199 = select <4 x i1> %198, <4 x float> %wide.load.27, <4 x float> zeroinitializer
  %200 = getelementptr inbounds float, float* %3, i64 %195
  %201 = bitcast float* %200 to <4 x float>*
  store <4 x float> %199, <4 x float>* %201, align 4, !tbaa !115
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 112
  br i1 %exitcond3, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 128
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !34
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
  %6 = mul nuw nsw i64 %indvars.iv4, 12544
  %7 = getelementptr inbounds float, float* %3, i64 %indvars.iv4
  %8 = load float, float* %7, align 4, !tbaa !118
  %broadcast.splatinsert7 = insertelement <4 x float> undef, float %8, i32 0
  %broadcast.splat8 = shufflevector <4 x float> %broadcast.splatinsert7, <4 x float> undef, <4 x i32> zeroinitializer
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv1 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next2, %for_begin4.preheader ]
  %9 = mul nuw nsw i64 %indvars.iv1, 112
  %10 = add nuw nsw i64 %9, %6
  %11 = getelementptr inbounds float, float* %4, i64 %10
  %12 = bitcast float* %11 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %12, align 4, !tbaa !121
  %13 = fadd <4 x float> %broadcast.splat8, %wide.load
  %14 = getelementptr inbounds float, float* %5, i64 %10
  %15 = bitcast float* %14 to <4 x float>*
  store <4 x float> %13, <4 x float>* %15, align 4, !tbaa !124
  %16 = or i64 %10, 4
  %17 = getelementptr inbounds float, float* %4, i64 %16
  %18 = bitcast float* %17 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %18, align 4, !tbaa !121
  %19 = fadd <4 x float> %broadcast.splat8, %wide.load.1
  %20 = getelementptr inbounds float, float* %5, i64 %16
  %21 = bitcast float* %20 to <4 x float>*
  store <4 x float> %19, <4 x float>* %21, align 4, !tbaa !124
  %22 = or i64 %10, 8
  %23 = getelementptr inbounds float, float* %4, i64 %22
  %24 = bitcast float* %23 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %24, align 4, !tbaa !121
  %25 = fadd <4 x float> %broadcast.splat8, %wide.load.2
  %26 = getelementptr inbounds float, float* %5, i64 %22
  %27 = bitcast float* %26 to <4 x float>*
  store <4 x float> %25, <4 x float>* %27, align 4, !tbaa !124
  %28 = or i64 %10, 12
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
  %90 = bitcast float* %89 to <4 x float>*
  %wide.load.13 = load <4 x float>, <4 x float>* %90, align 4, !tbaa !121
  %91 = fadd <4 x float> %broadcast.splat8, %wide.load.13
  %92 = getelementptr inbounds float, float* %5, i64 %88
  %93 = bitcast float* %92 to <4 x float>*
  store <4 x float> %91, <4 x float>* %93, align 4, !tbaa !124
  %94 = add nuw nsw i64 %10, 56
  %95 = getelementptr inbounds float, float* %4, i64 %94
  %96 = bitcast float* %95 to <4 x float>*
  %wide.load.14 = load <4 x float>, <4 x float>* %96, align 4, !tbaa !121
  %97 = fadd <4 x float> %broadcast.splat8, %wide.load.14
  %98 = getelementptr inbounds float, float* %5, i64 %94
  %99 = bitcast float* %98 to <4 x float>*
  store <4 x float> %97, <4 x float>* %99, align 4, !tbaa !124
  %100 = add nuw nsw i64 %10, 60
  %101 = getelementptr inbounds float, float* %4, i64 %100
  %102 = bitcast float* %101 to <4 x float>*
  %wide.load.15 = load <4 x float>, <4 x float>* %102, align 4, !tbaa !121
  %103 = fadd <4 x float> %broadcast.splat8, %wide.load.15
  %104 = getelementptr inbounds float, float* %5, i64 %100
  %105 = bitcast float* %104 to <4 x float>*
  store <4 x float> %103, <4 x float>* %105, align 4, !tbaa !124
  %106 = add nuw nsw i64 %10, 64
  %107 = getelementptr inbounds float, float* %4, i64 %106
  %108 = bitcast float* %107 to <4 x float>*
  %wide.load.16 = load <4 x float>, <4 x float>* %108, align 4, !tbaa !121
  %109 = fadd <4 x float> %broadcast.splat8, %wide.load.16
  %110 = getelementptr inbounds float, float* %5, i64 %106
  %111 = bitcast float* %110 to <4 x float>*
  store <4 x float> %109, <4 x float>* %111, align 4, !tbaa !124
  %112 = add nuw nsw i64 %10, 68
  %113 = getelementptr inbounds float, float* %4, i64 %112
  %114 = bitcast float* %113 to <4 x float>*
  %wide.load.17 = load <4 x float>, <4 x float>* %114, align 4, !tbaa !121
  %115 = fadd <4 x float> %broadcast.splat8, %wide.load.17
  %116 = getelementptr inbounds float, float* %5, i64 %112
  %117 = bitcast float* %116 to <4 x float>*
  store <4 x float> %115, <4 x float>* %117, align 4, !tbaa !124
  %118 = add nuw nsw i64 %10, 72
  %119 = getelementptr inbounds float, float* %4, i64 %118
  %120 = bitcast float* %119 to <4 x float>*
  %wide.load.18 = load <4 x float>, <4 x float>* %120, align 4, !tbaa !121
  %121 = fadd <4 x float> %broadcast.splat8, %wide.load.18
  %122 = getelementptr inbounds float, float* %5, i64 %118
  %123 = bitcast float* %122 to <4 x float>*
  store <4 x float> %121, <4 x float>* %123, align 4, !tbaa !124
  %124 = add nuw nsw i64 %10, 76
  %125 = getelementptr inbounds float, float* %4, i64 %124
  %126 = bitcast float* %125 to <4 x float>*
  %wide.load.19 = load <4 x float>, <4 x float>* %126, align 4, !tbaa !121
  %127 = fadd <4 x float> %broadcast.splat8, %wide.load.19
  %128 = getelementptr inbounds float, float* %5, i64 %124
  %129 = bitcast float* %128 to <4 x float>*
  store <4 x float> %127, <4 x float>* %129, align 4, !tbaa !124
  %130 = add nuw nsw i64 %10, 80
  %131 = getelementptr inbounds float, float* %4, i64 %130
  %132 = bitcast float* %131 to <4 x float>*
  %wide.load.20 = load <4 x float>, <4 x float>* %132, align 4, !tbaa !121
  %133 = fadd <4 x float> %broadcast.splat8, %wide.load.20
  %134 = getelementptr inbounds float, float* %5, i64 %130
  %135 = bitcast float* %134 to <4 x float>*
  store <4 x float> %133, <4 x float>* %135, align 4, !tbaa !124
  %136 = add nuw nsw i64 %10, 84
  %137 = getelementptr inbounds float, float* %4, i64 %136
  %138 = bitcast float* %137 to <4 x float>*
  %wide.load.21 = load <4 x float>, <4 x float>* %138, align 4, !tbaa !121
  %139 = fadd <4 x float> %broadcast.splat8, %wide.load.21
  %140 = getelementptr inbounds float, float* %5, i64 %136
  %141 = bitcast float* %140 to <4 x float>*
  store <4 x float> %139, <4 x float>* %141, align 4, !tbaa !124
  %142 = add nuw nsw i64 %10, 88
  %143 = getelementptr inbounds float, float* %4, i64 %142
  %144 = bitcast float* %143 to <4 x float>*
  %wide.load.22 = load <4 x float>, <4 x float>* %144, align 4, !tbaa !121
  %145 = fadd <4 x float> %broadcast.splat8, %wide.load.22
  %146 = getelementptr inbounds float, float* %5, i64 %142
  %147 = bitcast float* %146 to <4 x float>*
  store <4 x float> %145, <4 x float>* %147, align 4, !tbaa !124
  %148 = add nuw nsw i64 %10, 92
  %149 = getelementptr inbounds float, float* %4, i64 %148
  %150 = bitcast float* %149 to <4 x float>*
  %wide.load.23 = load <4 x float>, <4 x float>* %150, align 4, !tbaa !121
  %151 = fadd <4 x float> %broadcast.splat8, %wide.load.23
  %152 = getelementptr inbounds float, float* %5, i64 %148
  %153 = bitcast float* %152 to <4 x float>*
  store <4 x float> %151, <4 x float>* %153, align 4, !tbaa !124
  %154 = add nuw nsw i64 %10, 96
  %155 = getelementptr inbounds float, float* %4, i64 %154
  %156 = bitcast float* %155 to <4 x float>*
  %wide.load.24 = load <4 x float>, <4 x float>* %156, align 4, !tbaa !121
  %157 = fadd <4 x float> %broadcast.splat8, %wide.load.24
  %158 = getelementptr inbounds float, float* %5, i64 %154
  %159 = bitcast float* %158 to <4 x float>*
  store <4 x float> %157, <4 x float>* %159, align 4, !tbaa !124
  %160 = add nuw nsw i64 %10, 100
  %161 = getelementptr inbounds float, float* %4, i64 %160
  %162 = bitcast float* %161 to <4 x float>*
  %wide.load.25 = load <4 x float>, <4 x float>* %162, align 4, !tbaa !121
  %163 = fadd <4 x float> %broadcast.splat8, %wide.load.25
  %164 = getelementptr inbounds float, float* %5, i64 %160
  %165 = bitcast float* %164 to <4 x float>*
  store <4 x float> %163, <4 x float>* %165, align 4, !tbaa !124
  %166 = add nuw nsw i64 %10, 104
  %167 = getelementptr inbounds float, float* %4, i64 %166
  %168 = bitcast float* %167 to <4 x float>*
  %wide.load.26 = load <4 x float>, <4 x float>* %168, align 4, !tbaa !121
  %169 = fadd <4 x float> %broadcast.splat8, %wide.load.26
  %170 = getelementptr inbounds float, float* %5, i64 %166
  %171 = bitcast float* %170 to <4 x float>*
  store <4 x float> %169, <4 x float>* %171, align 4, !tbaa !124
  %172 = add nuw nsw i64 %10, 108
  %173 = getelementptr inbounds float, float* %4, i64 %172
  %174 = bitcast float* %173 to <4 x float>*
  %wide.load.27 = load <4 x float>, <4 x float>* %174, align 4, !tbaa !121
  %175 = fadd <4 x float> %broadcast.splat8, %wide.load.27
  %176 = getelementptr inbounds float, float* %5, i64 %172
  %177 = bitcast float* %176 to <4 x float>*
  store <4 x float> %175, <4 x float>* %177, align 4, !tbaa !124
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 112
  br i1 %exitcond3, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 128
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !34
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
  %wide.load = load <4 x float>, <4 x float>* %7, align 4, !tbaa !127
  %8 = getelementptr inbounds float, float* %6, i64 4
  %9 = bitcast float* %8 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !127
  %10 = getelementptr inbounds float, float* %4, i64 %index
  %11 = bitcast float* %10 to <4 x float>*
  %wide.load3 = load <4 x float>, <4 x float>* %11, align 4, !tbaa !130
  %12 = getelementptr inbounds float, float* %10, i64 4
  %13 = bitcast float* %12 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %13, align 4, !tbaa !130
  %14 = fadd <4 x float> %wide.load, %wide.load3
  %15 = fadd <4 x float> %wide.load2, %wide.load4
  %16 = getelementptr inbounds float, float* %5, i64 %index
  %17 = bitcast float* %16 to <4 x float>*
  store <4 x float> %14, <4 x float>* %17, align 4, !tbaa !133
  %18 = getelementptr inbounds float, float* %16, i64 4
  %19 = bitcast float* %18 to <4 x float>*
  store <4 x float> %15, <4 x float>* %19, align 4, !tbaa !133
  %index.next = add i64 %index, 8
  %20 = icmp eq i64 %index.next, 4096
  br i1 %20, label %for_end, label %vector.body, !llvm.loop !136

for_end:                                          ; preds = %vector.body
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
  %12 = load float, float* %11, align 4, !tbaa !137
  %13 = fcmp olt float %12, 0xC7EFFFFFE0000000
  %14 = select i1 %13, float 0xC7EFFFFFE0000000, float %12
  %15 = or i64 %9, 1
  %16 = getelementptr inbounds float, float* %3, i64 %15
  %17 = load float, float* %16, align 4, !tbaa !137
  %18 = fcmp ogt float %14, %17
  %19 = select i1 %18, float %14, float %17
  %20 = add nuw nsw i64 %9, 14
  %21 = getelementptr inbounds float, float* %3, i64 %20
  %22 = load float, float* %21, align 4, !tbaa !137
  %23 = fcmp ogt float %19, %22
  %24 = select i1 %23, float %19, float %22
  %25 = add nuw nsw i64 %9, 15
  %26 = getelementptr inbounds float, float* %3, i64 %25
  %27 = load float, float* %26, align 4, !tbaa !137
  %28 = fcmp ogt float %24, %27
  %29 = select i1 %28, float %24, float %27
  store float %29, float* %10, align 4, !tbaa !140
  %30 = add nuw nsw i64 %7, 1
  %31 = getelementptr inbounds float, float* %2, i64 %30
  %32 = or i64 %9, 2
  %33 = getelementptr inbounds float, float* %3, i64 %32
  %34 = load float, float* %33, align 4, !tbaa !137
  %35 = fcmp olt float %34, 0xC7EFFFFFE0000000
  %36 = select i1 %35, float 0xC7EFFFFFE0000000, float %34
  %37 = or i64 %9, 3
  %38 = getelementptr inbounds float, float* %3, i64 %37
  %39 = load float, float* %38, align 4, !tbaa !137
  %40 = fcmp ogt float %36, %39
  %41 = select i1 %40, float %36, float %39
  %42 = add nuw nsw i64 %32, 14
  %43 = getelementptr inbounds float, float* %3, i64 %42
  %44 = load float, float* %43, align 4, !tbaa !137
  %45 = fcmp ogt float %41, %44
  %46 = select i1 %45, float %41, float %44
  %47 = add nuw nsw i64 %32, 15
  %48 = getelementptr inbounds float, float* %3, i64 %47
  %49 = load float, float* %48, align 4, !tbaa !137
  %50 = fcmp ogt float %46, %49
  %51 = select i1 %50, float %46, float %49
  store float %51, float* %31, align 4, !tbaa !140
  %52 = add nuw nsw i64 %7, 2
  %53 = getelementptr inbounds float, float* %2, i64 %52
  %54 = add nuw nsw i64 %9, 4
  %55 = getelementptr inbounds float, float* %3, i64 %54
  %56 = load float, float* %55, align 4, !tbaa !137
  %57 = fcmp olt float %56, 0xC7EFFFFFE0000000
  %58 = select i1 %57, float 0xC7EFFFFFE0000000, float %56
  %59 = add nuw nsw i64 %9, 5
  %60 = getelementptr inbounds float, float* %3, i64 %59
  %61 = load float, float* %60, align 4, !tbaa !137
  %62 = fcmp ogt float %58, %61
  %63 = select i1 %62, float %58, float %61
  %64 = add nuw nsw i64 %9, 18
  %65 = getelementptr inbounds float, float* %3, i64 %64
  %66 = load float, float* %65, align 4, !tbaa !137
  %67 = fcmp ogt float %63, %66
  %68 = select i1 %67, float %63, float %66
  %69 = add nuw nsw i64 %9, 19
  %70 = getelementptr inbounds float, float* %3, i64 %69
  %71 = load float, float* %70, align 4, !tbaa !137
  %72 = fcmp ogt float %68, %71
  %73 = select i1 %72, float %68, float %71
  store float %73, float* %53, align 4, !tbaa !140
  %74 = add nuw nsw i64 %7, 3
  %75 = getelementptr inbounds float, float* %2, i64 %74
  %76 = add nuw nsw i64 %9, 6
  %77 = getelementptr inbounds float, float* %3, i64 %76
  %78 = load float, float* %77, align 4, !tbaa !137
  %79 = fcmp olt float %78, 0xC7EFFFFFE0000000
  %80 = select i1 %79, float 0xC7EFFFFFE0000000, float %78
  %81 = add nuw nsw i64 %9, 7
  %82 = getelementptr inbounds float, float* %3, i64 %81
  %83 = load float, float* %82, align 4, !tbaa !137
  %84 = fcmp ogt float %80, %83
  %85 = select i1 %84, float %80, float %83
  %86 = add nuw nsw i64 %9, 20
  %87 = getelementptr inbounds float, float* %3, i64 %86
  %88 = load float, float* %87, align 4, !tbaa !137
  %89 = fcmp ogt float %85, %88
  %90 = select i1 %89, float %85, float %88
  %91 = add nuw nsw i64 %9, 21
  %92 = getelementptr inbounds float, float* %3, i64 %91
  %93 = load float, float* %92, align 4, !tbaa !137
  %94 = fcmp ogt float %90, %93
  %95 = select i1 %94, float %90, float %93
  store float %95, float* %75, align 4, !tbaa !140
  %96 = add nuw nsw i64 %7, 4
  %97 = getelementptr inbounds float, float* %2, i64 %96
  %98 = add nuw nsw i64 %9, 8
  %99 = getelementptr inbounds float, float* %3, i64 %98
  %100 = load float, float* %99, align 4, !tbaa !137
  %101 = fcmp olt float %100, 0xC7EFFFFFE0000000
  %102 = select i1 %101, float 0xC7EFFFFFE0000000, float %100
  %103 = add nuw nsw i64 %9, 9
  %104 = getelementptr inbounds float, float* %3, i64 %103
  %105 = load float, float* %104, align 4, !tbaa !137
  %106 = fcmp ogt float %102, %105
  %107 = select i1 %106, float %102, float %105
  %108 = add nuw nsw i64 %9, 22
  %109 = getelementptr inbounds float, float* %3, i64 %108
  %110 = load float, float* %109, align 4, !tbaa !137
  %111 = fcmp ogt float %107, %110
  %112 = select i1 %111, float %107, float %110
  %113 = add nuw nsw i64 %9, 23
  %114 = getelementptr inbounds float, float* %3, i64 %113
  %115 = load float, float* %114, align 4, !tbaa !137
  %116 = fcmp ogt float %112, %115
  %117 = select i1 %116, float %112, float %115
  store float %117, float* %97, align 4, !tbaa !140
  %118 = add nuw nsw i64 %7, 5
  %119 = getelementptr inbounds float, float* %2, i64 %118
  %120 = add nuw nsw i64 %9, 10
  %121 = getelementptr inbounds float, float* %3, i64 %120
  %122 = load float, float* %121, align 4, !tbaa !137
  %123 = fcmp olt float %122, 0xC7EFFFFFE0000000
  %124 = select i1 %123, float 0xC7EFFFFFE0000000, float %122
  %125 = add nuw nsw i64 %9, 11
  %126 = getelementptr inbounds float, float* %3, i64 %125
  %127 = load float, float* %126, align 4, !tbaa !137
  %128 = fcmp ogt float %124, %127
  %129 = select i1 %128, float %124, float %127
  %130 = add nuw nsw i64 %9, 24
  %131 = getelementptr inbounds float, float* %3, i64 %130
  %132 = load float, float* %131, align 4, !tbaa !137
  %133 = fcmp ogt float %129, %132
  %134 = select i1 %133, float %129, float %132
  %135 = add nuw nsw i64 %9, 25
  %136 = getelementptr inbounds float, float* %3, i64 %135
  %137 = load float, float* %136, align 4, !tbaa !137
  %138 = fcmp ogt float %134, %137
  %139 = select i1 %138, float %134, float %137
  store float %139, float* %119, align 4, !tbaa !140
  %140 = add nuw nsw i64 %7, 6
  %141 = getelementptr inbounds float, float* %2, i64 %140
  %142 = add nuw nsw i64 %9, 12
  %143 = getelementptr inbounds float, float* %3, i64 %142
  %144 = load float, float* %143, align 4, !tbaa !137
  %145 = fcmp olt float %144, 0xC7EFFFFFE0000000
  %146 = select i1 %145, float 0xC7EFFFFFE0000000, float %144
  %147 = add nuw nsw i64 %9, 13
  %148 = getelementptr inbounds float, float* %3, i64 %147
  %149 = load float, float* %148, align 4, !tbaa !137
  %150 = fcmp ogt float %146, %149
  %151 = select i1 %150, float %146, float %149
  %152 = add nuw nsw i64 %9, 26
  %153 = getelementptr inbounds float, float* %3, i64 %152
  %154 = load float, float* %153, align 4, !tbaa !137
  %155 = fcmp ogt float %151, %154
  %156 = select i1 %155, float %151, float %154
  %157 = add nuw nsw i64 %9, 27
  %158 = getelementptr inbounds float, float* %3, i64 %157
  %159 = load float, float* %158, align 4, !tbaa !137
  %160 = fcmp ogt float %156, %159
  %161 = select i1 %160, float %156, float %159
  store float %161, float* %141, align 4, !tbaa !140
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 7
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 512
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !34
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
  %10 = load <16 x float>, <16 x float>* %9, align 64, !tbaa !143
  %11 = add nuw nsw i64 %7, %6
  %12 = getelementptr inbounds float, float* %4, i64 %11
  %13 = bitcast float* %12 to <16 x float>*
  %14 = load <16 x float>, <16 x float>* %13, align 64, !tbaa !146
  %15 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %10, <16 x float> %14, <16 x float> %.02)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !34

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
  store float %32, float* %16, align 4, !tbaa !149
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 1000
  br i1 %exitcond5, label %for_end, label %for_begin1.preheader, !prof !34
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
  %5 = load float, float* %4, align 64, !tbaa !152
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
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !166
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !166
  %11 = fmul <4 x float> %wide.load, %broadcast.splat4
  %12 = fmul <4 x float> %wide.load2, %broadcast.splat6
  %13 = getelementptr inbounds float, float* %6, i64 %index
  %14 = bitcast float* %13 to <4 x float>*
  store <4 x float> %11, <4 x float>* %14, align 4, !tbaa !169
  %15 = getelementptr inbounds float, float* %13, i64 4
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %12, <4 x float>* %16, align 4, !tbaa !169
  %index.next = add i64 %index, 8
  %17 = icmp eq i64 %index.next, 25088
  br i1 %17, label %for_end, label %vector.body, !llvm.loop !172

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
  %3 = alloca [14 x <16 x float>], align 64
  %4 = alloca [147456 x <16 x float>], align 16
  %5 = alloca [131072 x float], align 16
  %6 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar = phi i64 [ 0, %entry ], [ %indvar.next, %for_end3 ]
  %7 = shl i64 %indvar, 13
  %8 = trunc i64 %indvar to i32
  %9 = add i32 %8, -1
  %10 = icmp ult i32 %9, 14
  %11 = mul nuw nsw i64 %indvar, 14
  %12 = add nsw i64 %11, -15
  br i1 %10, label %if_end.us.us.15, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep99 = getelementptr [131072 x float], [131072 x float]* %5, i64 0, i64 %7
  %scevgep99100 = bitcast float* %scevgep99 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep99100, i8 0, i64 32768, i1 false)
  br label %for_end3

for_begin7.preheader:                             ; preds = %for_end3
  %.sub = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0
  %13 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_end3:                                         ; preds = %if_end.us.us.15, %for_begin4.preheader.preheader
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond108 = icmp eq i64 %indvar.next, 16
  br i1 %exitcond108, label %for_begin7.preheader, label %for_begin1.preheader, !prof !34

for_begin10.preheader:                            ; preds = %for_end15.2, %for_begin7.preheader
  %indvars.iv89 = phi i64 [ 0, %for_begin7.preheader ], [ %indvars.iv.next90, %for_end15.2 ]
  %14 = mul nuw nsw i64 %indvars.iv89, 24576
  %15 = trunc i64 %indvars.iv89 to i32
  %16 = urem i32 %15, 3
  %17 = mul nuw nsw i32 %16, 3
  %18 = udiv i32 %15, 3
  %19 = mul nsw i32 %18, 73728
  %20 = or i32 %17, %19
  %21 = zext i32 %20 to i64
  br label %for_body14

for_begin16.preheader:                            ; preds = %for_end15.2
  %22 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 16
  %23 = bitcast float* %22 to <16 x float>*
  %24 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 32
  %25 = bitcast float* %24 to <16 x float>*
  %26 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 48
  %27 = bitcast float* %26 to <16 x float>*
  %28 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 64
  %29 = bitcast float* %28 to <16 x float>*
  %30 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 80
  %31 = bitcast float* %30 to <16 x float>*
  %32 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 96
  %33 = bitcast float* %32 to <16 x float>*
  %34 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 112
  %35 = bitcast float* %34 to <16 x float>*
  %36 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 128
  %37 = bitcast float* %36 to <16 x float>*
  %38 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 144
  %39 = bitcast float* %38 to <16 x float>*
  %40 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 160
  %41 = bitcast float* %40 to <16 x float>*
  %42 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 176
  %43 = bitcast float* %42 to <16 x float>*
  %44 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 192
  %45 = bitcast float* %44 to <16 x float>*
  %46 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 208
  %47 = bitcast float* %46 to <16 x float>*
  %48 = bitcast i8* %2 to float*
  %49 = bitcast [14 x <16 x float>]* %3 to i8*
  br label %for_body17

for_body14:                                       ; preds = %for_body14, %for_begin10.preheader
  %indvars.iv83 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next84, %for_body14 ]
  %50 = shl i64 %indvars.iv83, 4
  %51 = add nuw nsw i64 %14, %50
  %52 = mul nuw nsw i64 %indvars.iv83, 9
  %53 = add nuw nsw i64 %52, %21
  %54 = add nuw nsw i64 %53, 4608
  %55 = add nuw nsw i64 %53, 9216
  %56 = add nuw nsw i64 %53, 13824
  %57 = add nuw nsw i64 %53, 18432
  %58 = add nuw nsw i64 %53, 23040
  %59 = add nuw nsw i64 %53, 27648
  %60 = add nuw nsw i64 %53, 32256
  %61 = add nuw nsw i64 %53, 36864
  %62 = add nuw nsw i64 %53, 41472
  %63 = add nuw nsw i64 %53, 46080
  %64 = add nuw nsw i64 %53, 50688
  %65 = add nuw nsw i64 %53, 55296
  %66 = add nuw nsw i64 %53, 59904
  %67 = add nuw nsw i64 %53, 64512
  %68 = add nuw nsw i64 %53, 69120
  %69 = getelementptr inbounds float, float* %13, i64 %53
  %70 = load float, float* %69, align 4, !tbaa !173
  %71 = insertelement <16 x float> undef, float %70, i32 0
  %72 = getelementptr inbounds float, float* %13, i64 %54
  %73 = load float, float* %72, align 4, !tbaa !173
  %74 = insertelement <16 x float> %71, float %73, i32 1
  %75 = getelementptr inbounds float, float* %13, i64 %55
  %76 = load float, float* %75, align 4, !tbaa !173
  %77 = insertelement <16 x float> %74, float %76, i32 2
  %78 = getelementptr inbounds float, float* %13, i64 %56
  %79 = load float, float* %78, align 4, !tbaa !173
  %80 = insertelement <16 x float> %77, float %79, i32 3
  %81 = getelementptr inbounds float, float* %13, i64 %57
  %82 = load float, float* %81, align 4, !tbaa !173
  %83 = insertelement <16 x float> %80, float %82, i32 4
  %84 = getelementptr inbounds float, float* %13, i64 %58
  %85 = load float, float* %84, align 4, !tbaa !173
  %86 = insertelement <16 x float> %83, float %85, i32 5
  %87 = getelementptr inbounds float, float* %13, i64 %59
  %88 = load float, float* %87, align 4, !tbaa !173
  %89 = insertelement <16 x float> %86, float %88, i32 6
  %90 = getelementptr inbounds float, float* %13, i64 %60
  %91 = load float, float* %90, align 4, !tbaa !173
  %92 = insertelement <16 x float> %89, float %91, i32 7
  %93 = getelementptr inbounds float, float* %13, i64 %61
  %94 = load float, float* %93, align 4, !tbaa !173
  %95 = insertelement <16 x float> %92, float %94, i32 8
  %96 = getelementptr inbounds float, float* %13, i64 %62
  %97 = load float, float* %96, align 4, !tbaa !173
  %98 = insertelement <16 x float> %95, float %97, i32 9
  %99 = getelementptr inbounds float, float* %13, i64 %63
  %100 = load float, float* %99, align 4, !tbaa !173
  %101 = insertelement <16 x float> %98, float %100, i32 10
  %102 = getelementptr inbounds float, float* %13, i64 %64
  %103 = load float, float* %102, align 4, !tbaa !173
  %104 = insertelement <16 x float> %101, float %103, i32 11
  %105 = getelementptr inbounds float, float* %13, i64 %65
  %106 = load float, float* %105, align 4, !tbaa !173
  %107 = insertelement <16 x float> %104, float %106, i32 12
  %108 = getelementptr inbounds float, float* %13, i64 %66
  %109 = load float, float* %108, align 4, !tbaa !173
  %110 = insertelement <16 x float> %107, float %109, i32 13
  %111 = getelementptr inbounds float, float* %13, i64 %67
  %112 = load float, float* %111, align 4, !tbaa !173
  %113 = insertelement <16 x float> %110, float %112, i32 14
  %114 = getelementptr inbounds float, float* %13, i64 %68
  %115 = load float, float* %114, align 4, !tbaa !173
  %116 = insertelement <16 x float> %113, float %115, i32 15
  %117 = getelementptr inbounds [147456 x <16 x float>], [147456 x <16 x float>]* %4, i64 0, i64 0, i64 %51
  %118 = bitcast float* %117 to <16 x float>*
  store <16 x float> %116, <16 x float>* %118, align 16, !tbaa !176
  %indvars.iv.next84 = add nuw nsw i64 %indvars.iv83, 1
  %exitcond85 = icmp eq i64 %indvars.iv.next84, 512
  br i1 %exitcond85, label %for_end15, label %for_body14, !prof !34

for_end15:                                        ; preds = %for_body14
  %119 = add nuw nsw i64 %14, 8192
  %120 = add nuw nsw i64 %21, 1
  br label %for_body14.1

for_body17:                                       ; preds = %for_end27, %for_begin16.preheader
  %121 = phi i32 [ 0, %for_begin16.preheader ], [ %357, %for_end27 ]
  %122 = urem i32 %121, 14
  %123 = udiv i32 %121, 14
  %124 = mul nsw i32 %123, 73728
  %125 = zext i32 %124 to i64
  call void @llvm.memset.p0i8.i64(i8* nonnull align 64 %49, i8 0, i64 896, i1 false)
  br label %for_begin22.preheader

for_end18:                                        ; preds = %for_end27
  ret void

for_begin25.preheader:                            ; preds = %for_end24
  store <16 x float> %283, <16 x float>* %.sub, align 64, !tbaa !179
  store <16 x float> %284, <16 x float>* %23, align 64, !tbaa !179
  store <16 x float> %285, <16 x float>* %25, align 64, !tbaa !179
  store <16 x float> %286, <16 x float>* %27, align 64, !tbaa !179
  store <16 x float> %287, <16 x float>* %29, align 64, !tbaa !179
  store <16 x float> %288, <16 x float>* %31, align 64, !tbaa !179
  store <16 x float> %289, <16 x float>* %33, align 64, !tbaa !179
  store <16 x float> %290, <16 x float>* %35, align 64, !tbaa !179
  store <16 x float> %291, <16 x float>* %37, align 64, !tbaa !179
  store <16 x float> %292, <16 x float>* %39, align 64, !tbaa !179
  store <16 x float> %293, <16 x float>* %41, align 64, !tbaa !179
  store <16 x float> %294, <16 x float>* %43, align 64, !tbaa !179
  store <16 x float> %295, <16 x float>* %45, align 64, !tbaa !179
  store <16 x float> %302, <16 x float>* %47, align 64, !tbaa !179
  %126 = mul nuw nsw i32 %122, 14
  %127 = mul nsw i32 %123, 3136
  %128 = add nuw nsw i32 %127, %126
  %129 = zext i32 %128 to i64
  br label %for_body26

for_begin22.preheader:                            ; preds = %for_end24, %for_body17
  %indvars.iv76 = phi i64 [ 0, %for_body17 ], [ %indvars.iv.next77, %for_end24 ]
  %.lcssa2754 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %302, %for_end24 ]
  %.lcssa2552 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %295, %for_end24 ]
  %.lcssa2350 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %294, %for_end24 ]
  %.lcssa2148 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %293, %for_end24 ]
  %.lcssa1946 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %292, %for_end24 ]
  %.lcssa1744 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %291, %for_end24 ]
  %.lcssa1542 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %290, %for_end24 ]
  %.lcssa1340 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %289, %for_end24 ]
  %.lcssa1138 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %288, %for_end24 ]
  %.lcssa936 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %287, %for_end24 ]
  %.lcssa734 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %286, %for_end24 ]
  %.lcssa532 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %285, %for_end24 ]
  %.lcssa331 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %284, %for_end24 ]
  %.lcssa29 = phi <16 x float> [ zeroinitializer, %for_body17 ], [ %283, %for_end24 ]
  %130 = phi i32 [ 0, %for_body17 ], [ %304, %for_end24 ]
  %131 = add nuw nsw i32 %130, %122
  %132 = shl i32 %131, 13
  %133 = mul nuw nsw i64 %indvars.iv76, 24576
  %134 = add nuw nsw i64 %133, %125
  %135 = sext i32 %132 to i64
  br label %for_body23

for_body23:                                       ; preds = %for_body23, %for_begin22.preheader
  %indvars.iv = phi i64 [ 0, %for_begin22.preheader ], [ %indvars.iv.next, %for_body23 ]
  %136 = phi <16 x float> [ %.lcssa2754, %for_begin22.preheader ], [ %302, %for_body23 ]
  %137 = phi <16 x float> [ %.lcssa2552, %for_begin22.preheader ], [ %295, %for_body23 ]
  %138 = phi <16 x float> [ %.lcssa2350, %for_begin22.preheader ], [ %294, %for_body23 ]
  %139 = phi <16 x float> [ %.lcssa2148, %for_begin22.preheader ], [ %293, %for_body23 ]
  %140 = phi <16 x float> [ %.lcssa1946, %for_begin22.preheader ], [ %292, %for_body23 ]
  %141 = phi <16 x float> [ %.lcssa1744, %for_begin22.preheader ], [ %291, %for_body23 ]
  %142 = phi <16 x float> [ %.lcssa1542, %for_begin22.preheader ], [ %290, %for_body23 ]
  %143 = phi <16 x float> [ %.lcssa1340, %for_begin22.preheader ], [ %289, %for_body23 ]
  %144 = phi <16 x float> [ %.lcssa1138, %for_begin22.preheader ], [ %288, %for_body23 ]
  %145 = phi <16 x float> [ %.lcssa936, %for_begin22.preheader ], [ %287, %for_body23 ]
  %146 = phi <16 x float> [ %.lcssa734, %for_begin22.preheader ], [ %286, %for_body23 ]
  %147 = phi <16 x float> [ %.lcssa532, %for_begin22.preheader ], [ %285, %for_body23 ]
  %148 = phi <16 x float> [ %.lcssa331, %for_begin22.preheader ], [ %284, %for_body23 ]
  %149 = phi <16 x float> [ %.lcssa29, %for_begin22.preheader ], [ %283, %for_body23 ]
  %150 = phi i32 [ 0, %for_begin22.preheader ], [ %303, %for_body23 ]
  %151 = shl nsw i64 %indvars.iv, 4
  %152 = shl nsw i32 %150, 4
  %153 = add nsw i64 %151, %135
  %154 = add nsw i32 %152, %132
  %155 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %153
  %156 = load float, float* %155, align 16, !tbaa !189
  %157 = insertelement <16 x float> undef, float %156, i32 0
  %158 = shufflevector <16 x float> %157, <16 x float> undef, <16 x i32> zeroinitializer
  %159 = add nuw nsw i64 %134, %151
  %160 = getelementptr inbounds [147456 x <16 x float>], [147456 x <16 x float>]* %4, i64 0, i64 0, i64 %159
  %161 = bitcast float* %160 to <16 x float>*
  %162 = load <16 x float>, <16 x float>* %161, align 16, !tbaa !176
  %163 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %158, <16 x float> %162, <16 x float> %149)
  %164 = or i32 %154, 1
  %165 = sext i32 %164 to i64
  %166 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %165
  %167 = load float, float* %166, align 4, !tbaa !189
  %168 = insertelement <16 x float> undef, float %167, i32 0
  %169 = shufflevector <16 x float> %168, <16 x float> undef, <16 x i32> zeroinitializer
  %170 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %169, <16 x float> %162, <16 x float> %148)
  %171 = or i32 %154, 2
  %172 = sext i32 %171 to i64
  %173 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %172
  %174 = load float, float* %173, align 8, !tbaa !189
  %175 = insertelement <16 x float> undef, float %174, i32 0
  %176 = shufflevector <16 x float> %175, <16 x float> undef, <16 x i32> zeroinitializer
  %177 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %176, <16 x float> %162, <16 x float> %147)
  %178 = or i32 %154, 3
  %179 = sext i32 %178 to i64
  %180 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %179
  %181 = load float, float* %180, align 4, !tbaa !189
  %182 = insertelement <16 x float> undef, float %181, i32 0
  %183 = shufflevector <16 x float> %182, <16 x float> undef, <16 x i32> zeroinitializer
  %184 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %183, <16 x float> %162, <16 x float> %146)
  %185 = or i32 %154, 4
  %186 = sext i32 %185 to i64
  %187 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %186
  %188 = load float, float* %187, align 16, !tbaa !189
  %189 = insertelement <16 x float> undef, float %188, i32 0
  %190 = shufflevector <16 x float> %189, <16 x float> undef, <16 x i32> zeroinitializer
  %191 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %190, <16 x float> %162, <16 x float> %145)
  %192 = or i32 %154, 5
  %193 = sext i32 %192 to i64
  %194 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %193
  %195 = load float, float* %194, align 4, !tbaa !189
  %196 = insertelement <16 x float> undef, float %195, i32 0
  %197 = shufflevector <16 x float> %196, <16 x float> undef, <16 x i32> zeroinitializer
  %198 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %197, <16 x float> %162, <16 x float> %144)
  %199 = or i32 %154, 6
  %200 = sext i32 %199 to i64
  %201 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %200
  %202 = load float, float* %201, align 8, !tbaa !189
  %203 = insertelement <16 x float> undef, float %202, i32 0
  %204 = shufflevector <16 x float> %203, <16 x float> undef, <16 x i32> zeroinitializer
  %205 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %204, <16 x float> %162, <16 x float> %143)
  %206 = or i32 %154, 7
  %207 = sext i32 %206 to i64
  %208 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %207
  %209 = load float, float* %208, align 4, !tbaa !189
  %210 = insertelement <16 x float> undef, float %209, i32 0
  %211 = shufflevector <16 x float> %210, <16 x float> undef, <16 x i32> zeroinitializer
  %212 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %211, <16 x float> %162, <16 x float> %142)
  %213 = or i32 %154, 8
  %214 = sext i32 %213 to i64
  %215 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %214
  %216 = load float, float* %215, align 16, !tbaa !189
  %217 = insertelement <16 x float> undef, float %216, i32 0
  %218 = shufflevector <16 x float> %217, <16 x float> undef, <16 x i32> zeroinitializer
  %219 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %218, <16 x float> %162, <16 x float> %141)
  %220 = or i32 %154, 9
  %221 = sext i32 %220 to i64
  %222 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %221
  %223 = load float, float* %222, align 4, !tbaa !189
  %224 = insertelement <16 x float> undef, float %223, i32 0
  %225 = shufflevector <16 x float> %224, <16 x float> undef, <16 x i32> zeroinitializer
  %226 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %225, <16 x float> %162, <16 x float> %140)
  %227 = or i32 %154, 10
  %228 = sext i32 %227 to i64
  %229 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %228
  %230 = load float, float* %229, align 8, !tbaa !189
  %231 = insertelement <16 x float> undef, float %230, i32 0
  %232 = shufflevector <16 x float> %231, <16 x float> undef, <16 x i32> zeroinitializer
  %233 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %232, <16 x float> %162, <16 x float> %139)
  %234 = or i32 %154, 11
  %235 = sext i32 %234 to i64
  %236 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %235
  %237 = load float, float* %236, align 4, !tbaa !189
  %238 = insertelement <16 x float> undef, float %237, i32 0
  %239 = shufflevector <16 x float> %238, <16 x float> undef, <16 x i32> zeroinitializer
  %240 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %239, <16 x float> %162, <16 x float> %138)
  %241 = or i32 %154, 12
  %242 = sext i32 %241 to i64
  %243 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %242
  %244 = load float, float* %243, align 16, !tbaa !189
  %245 = insertelement <16 x float> undef, float %244, i32 0
  %246 = shufflevector <16 x float> %245, <16 x float> undef, <16 x i32> zeroinitializer
  %247 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %246, <16 x float> %162, <16 x float> %137)
  %248 = or i32 %154, 13
  %249 = sext i32 %248 to i64
  %250 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %249
  %251 = load float, float* %250, align 4, !tbaa !189
  %252 = insertelement <16 x float> undef, float %251, i32 0
  %253 = shufflevector <16 x float> %252, <16 x float> undef, <16 x i32> zeroinitializer
  %254 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %253, <16 x float> %162, <16 x float> %136)
  %255 = add nuw nsw i64 %159, 8192
  %256 = getelementptr inbounds [147456 x <16 x float>], [147456 x <16 x float>]* %4, i64 0, i64 0, i64 %255
  %257 = bitcast float* %256 to <16 x float>*
  %258 = load <16 x float>, <16 x float>* %257, align 16, !tbaa !176
  %259 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %169, <16 x float> %258, <16 x float> %163)
  %260 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %176, <16 x float> %258, <16 x float> %170)
  %261 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %183, <16 x float> %258, <16 x float> %177)
  %262 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %190, <16 x float> %258, <16 x float> %184)
  %263 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %197, <16 x float> %258, <16 x float> %191)
  %264 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %204, <16 x float> %258, <16 x float> %198)
  %265 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %211, <16 x float> %258, <16 x float> %205)
  %266 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %218, <16 x float> %258, <16 x float> %212)
  %267 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %225, <16 x float> %258, <16 x float> %219)
  %268 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %232, <16 x float> %258, <16 x float> %226)
  %269 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %239, <16 x float> %258, <16 x float> %233)
  %270 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %246, <16 x float> %258, <16 x float> %240)
  %271 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %253, <16 x float> %258, <16 x float> %247)
  %272 = or i32 %154, 14
  %273 = sext i32 %272 to i64
  %274 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %273
  %275 = load float, float* %274, align 8, !tbaa !189
  %276 = insertelement <16 x float> undef, float %275, i32 0
  %277 = shufflevector <16 x float> %276, <16 x float> undef, <16 x i32> zeroinitializer
  %278 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %277, <16 x float> %258, <16 x float> %254)
  %279 = add nuw nsw i64 %159, 16384
  %280 = getelementptr inbounds [147456 x <16 x float>], [147456 x <16 x float>]* %4, i64 0, i64 0, i64 %279
  %281 = bitcast float* %280 to <16 x float>*
  %282 = load <16 x float>, <16 x float>* %281, align 16, !tbaa !176
  %283 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %176, <16 x float> %282, <16 x float> %259)
  %284 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %183, <16 x float> %282, <16 x float> %260)
  %285 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %190, <16 x float> %282, <16 x float> %261)
  %286 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %197, <16 x float> %282, <16 x float> %262)
  %287 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %204, <16 x float> %282, <16 x float> %263)
  %288 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %211, <16 x float> %282, <16 x float> %264)
  %289 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %218, <16 x float> %282, <16 x float> %265)
  %290 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %225, <16 x float> %282, <16 x float> %266)
  %291 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %232, <16 x float> %282, <16 x float> %267)
  %292 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %239, <16 x float> %282, <16 x float> %268)
  %293 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %246, <16 x float> %282, <16 x float> %269)
  %294 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %253, <16 x float> %282, <16 x float> %270)
  %295 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %277, <16 x float> %282, <16 x float> %271)
  %296 = or i32 %154, 15
  %297 = sext i32 %296 to i64
  %298 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %297
  %299 = load float, float* %298, align 4, !tbaa !189
  %300 = insertelement <16 x float> undef, float %299, i32 0
  %301 = shufflevector <16 x float> %300, <16 x float> undef, <16 x i32> zeroinitializer
  %302 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %301, <16 x float> %282, <16 x float> %278)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %303 = add nuw nsw i32 %150, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 512
  br i1 %exitcond, label %for_end24, label %for_body23, !prof !34

for_end24:                                        ; preds = %for_body23
  %indvars.iv.next77 = add nuw nsw i64 %indvars.iv76, 1
  %304 = add nuw nsw i32 %130, 1
  %exitcond78 = icmp eq i64 %indvars.iv.next77, 3
  br i1 %exitcond78, label %for_begin25.preheader, label %for_begin22.preheader, !prof !34

for_body26:                                       ; preds = %for_body26, %for_begin25.preheader
  %indvars.iv79 = phi i64 [ 0, %for_begin25.preheader ], [ %indvars.iv.next80, %for_body26 ]
  %305 = add nuw nsw i64 %indvars.iv79, %129
  %306 = add nuw nsw i64 %305, 196
  %307 = add nuw nsw i64 %305, 392
  %308 = add nuw nsw i64 %305, 588
  %309 = add nuw nsw i64 %305, 784
  %310 = add nuw nsw i64 %305, 980
  %311 = add nuw nsw i64 %305, 1176
  %312 = add nuw nsw i64 %305, 1372
  %313 = add nuw nsw i64 %305, 1568
  %314 = add nuw nsw i64 %305, 1764
  %315 = add nuw nsw i64 %305, 1960
  %316 = add nuw nsw i64 %305, 2156
  %317 = add nuw nsw i64 %305, 2352
  %318 = add nuw nsw i64 %305, 2548
  %319 = add nuw nsw i64 %305, 2744
  %320 = add nuw nsw i64 %305, 2940
  %321 = shl nsw i64 %indvars.iv79, 4
  %322 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 %321
  %323 = bitcast float* %322 to <16 x float>*
  %324 = load <16 x float>, <16 x float>* %323, align 64, !tbaa !192
  %325 = getelementptr inbounds float, float* %48, i64 %305
  %326 = extractelement <16 x float> %324, i64 0
  store float %326, float* %325, align 4, !tbaa !193
  %327 = getelementptr inbounds float, float* %48, i64 %306
  %328 = extractelement <16 x float> %324, i64 1
  store float %328, float* %327, align 4, !tbaa !193
  %329 = getelementptr inbounds float, float* %48, i64 %307
  %330 = extractelement <16 x float> %324, i64 2
  store float %330, float* %329, align 4, !tbaa !193
  %331 = getelementptr inbounds float, float* %48, i64 %308
  %332 = extractelement <16 x float> %324, i64 3
  store float %332, float* %331, align 4, !tbaa !193
  %333 = getelementptr inbounds float, float* %48, i64 %309
  %334 = extractelement <16 x float> %324, i64 4
  store float %334, float* %333, align 4, !tbaa !193
  %335 = getelementptr inbounds float, float* %48, i64 %310
  %336 = extractelement <16 x float> %324, i64 5
  store float %336, float* %335, align 4, !tbaa !193
  %337 = getelementptr inbounds float, float* %48, i64 %311
  %338 = extractelement <16 x float> %324, i64 6
  store float %338, float* %337, align 4, !tbaa !193
  %339 = getelementptr inbounds float, float* %48, i64 %312
  %340 = extractelement <16 x float> %324, i64 7
  store float %340, float* %339, align 4, !tbaa !193
  %341 = getelementptr inbounds float, float* %48, i64 %313
  %342 = extractelement <16 x float> %324, i64 8
  store float %342, float* %341, align 4, !tbaa !193
  %343 = getelementptr inbounds float, float* %48, i64 %314
  %344 = extractelement <16 x float> %324, i64 9
  store float %344, float* %343, align 4, !tbaa !193
  %345 = getelementptr inbounds float, float* %48, i64 %315
  %346 = extractelement <16 x float> %324, i64 10
  store float %346, float* %345, align 4, !tbaa !193
  %347 = getelementptr inbounds float, float* %48, i64 %316
  %348 = extractelement <16 x float> %324, i64 11
  store float %348, float* %347, align 4, !tbaa !193
  %349 = getelementptr inbounds float, float* %48, i64 %317
  %350 = extractelement <16 x float> %324, i64 12
  store float %350, float* %349, align 4, !tbaa !193
  %351 = getelementptr inbounds float, float* %48, i64 %318
  %352 = extractelement <16 x float> %324, i64 13
  store float %352, float* %351, align 4, !tbaa !193
  %353 = getelementptr inbounds float, float* %48, i64 %319
  %354 = extractelement <16 x float> %324, i64 14
  store float %354, float* %353, align 4, !tbaa !193
  %355 = getelementptr inbounds float, float* %48, i64 %320
  %356 = extractelement <16 x float> %324, i64 15
  store float %356, float* %355, align 4, !tbaa !193
  %indvars.iv.next80 = add nuw nsw i64 %indvars.iv79, 1
  %exitcond81 = icmp eq i64 %indvars.iv.next80, 14
  br i1 %exitcond81, label %for_end27, label %for_body26, !prof !34

for_end27:                                        ; preds = %for_body26
  %357 = add nuw nsw i32 %121, 1
  %exitcond82 = icmp eq i32 %357, 448
  br i1 %exitcond82, label %for_end18, label %for_body17, !prof !34

for_body14.1:                                     ; preds = %for_body14.1, %for_end15
  %indvars.iv83.1 = phi i64 [ 0, %for_end15 ], [ %indvars.iv.next84.1, %for_body14.1 ]
  %358 = shl i64 %indvars.iv83.1, 4
  %359 = add nuw nsw i64 %119, %358
  %360 = mul nuw nsw i64 %indvars.iv83.1, 9
  %361 = add nuw nsw i64 %120, %360
  %362 = add nuw nsw i64 %361, 4608
  %363 = add nuw nsw i64 %361, 9216
  %364 = add nuw nsw i64 %361, 13824
  %365 = add nuw nsw i64 %361, 18432
  %366 = add nuw nsw i64 %361, 23040
  %367 = add nuw nsw i64 %361, 27648
  %368 = add nuw nsw i64 %361, 32256
  %369 = add nuw nsw i64 %361, 36864
  %370 = add nuw nsw i64 %361, 41472
  %371 = add nuw nsw i64 %361, 46080
  %372 = add nuw nsw i64 %361, 50688
  %373 = add nuw nsw i64 %361, 55296
  %374 = add nuw nsw i64 %361, 59904
  %375 = add nuw nsw i64 %361, 64512
  %376 = add nuw nsw i64 %361, 69120
  %377 = getelementptr inbounds float, float* %13, i64 %361
  %378 = load float, float* %377, align 4, !tbaa !173
  %379 = insertelement <16 x float> undef, float %378, i32 0
  %380 = getelementptr inbounds float, float* %13, i64 %362
  %381 = load float, float* %380, align 4, !tbaa !173
  %382 = insertelement <16 x float> %379, float %381, i32 1
  %383 = getelementptr inbounds float, float* %13, i64 %363
  %384 = load float, float* %383, align 4, !tbaa !173
  %385 = insertelement <16 x float> %382, float %384, i32 2
  %386 = getelementptr inbounds float, float* %13, i64 %364
  %387 = load float, float* %386, align 4, !tbaa !173
  %388 = insertelement <16 x float> %385, float %387, i32 3
  %389 = getelementptr inbounds float, float* %13, i64 %365
  %390 = load float, float* %389, align 4, !tbaa !173
  %391 = insertelement <16 x float> %388, float %390, i32 4
  %392 = getelementptr inbounds float, float* %13, i64 %366
  %393 = load float, float* %392, align 4, !tbaa !173
  %394 = insertelement <16 x float> %391, float %393, i32 5
  %395 = getelementptr inbounds float, float* %13, i64 %367
  %396 = load float, float* %395, align 4, !tbaa !173
  %397 = insertelement <16 x float> %394, float %396, i32 6
  %398 = getelementptr inbounds float, float* %13, i64 %368
  %399 = load float, float* %398, align 4, !tbaa !173
  %400 = insertelement <16 x float> %397, float %399, i32 7
  %401 = getelementptr inbounds float, float* %13, i64 %369
  %402 = load float, float* %401, align 4, !tbaa !173
  %403 = insertelement <16 x float> %400, float %402, i32 8
  %404 = getelementptr inbounds float, float* %13, i64 %370
  %405 = load float, float* %404, align 4, !tbaa !173
  %406 = insertelement <16 x float> %403, float %405, i32 9
  %407 = getelementptr inbounds float, float* %13, i64 %371
  %408 = load float, float* %407, align 4, !tbaa !173
  %409 = insertelement <16 x float> %406, float %408, i32 10
  %410 = getelementptr inbounds float, float* %13, i64 %372
  %411 = load float, float* %410, align 4, !tbaa !173
  %412 = insertelement <16 x float> %409, float %411, i32 11
  %413 = getelementptr inbounds float, float* %13, i64 %373
  %414 = load float, float* %413, align 4, !tbaa !173
  %415 = insertelement <16 x float> %412, float %414, i32 12
  %416 = getelementptr inbounds float, float* %13, i64 %374
  %417 = load float, float* %416, align 4, !tbaa !173
  %418 = insertelement <16 x float> %415, float %417, i32 13
  %419 = getelementptr inbounds float, float* %13, i64 %375
  %420 = load float, float* %419, align 4, !tbaa !173
  %421 = insertelement <16 x float> %418, float %420, i32 14
  %422 = getelementptr inbounds float, float* %13, i64 %376
  %423 = load float, float* %422, align 4, !tbaa !173
  %424 = insertelement <16 x float> %421, float %423, i32 15
  %425 = getelementptr inbounds [147456 x <16 x float>], [147456 x <16 x float>]* %4, i64 0, i64 0, i64 %359
  %426 = bitcast float* %425 to <16 x float>*
  store <16 x float> %424, <16 x float>* %426, align 16, !tbaa !176
  %indvars.iv.next84.1 = add nuw nsw i64 %indvars.iv83.1, 1
  %exitcond85.1 = icmp eq i64 %indvars.iv.next84.1, 512
  br i1 %exitcond85.1, label %for_end15.1, label %for_body14.1, !prof !34

for_end15.1:                                      ; preds = %for_body14.1
  %427 = add nuw nsw i64 %14, 16384
  %428 = add nuw nsw i64 %21, 2
  br label %for_body14.2

for_body14.2:                                     ; preds = %for_body14.2, %for_end15.1
  %indvars.iv83.2 = phi i64 [ 0, %for_end15.1 ], [ %indvars.iv.next84.2, %for_body14.2 ]
  %429 = shl i64 %indvars.iv83.2, 4
  %430 = add nuw nsw i64 %427, %429
  %431 = mul nuw nsw i64 %indvars.iv83.2, 9
  %432 = add nuw nsw i64 %428, %431
  %433 = add nuw nsw i64 %432, 4608
  %434 = add nuw nsw i64 %432, 9216
  %435 = add nuw nsw i64 %432, 13824
  %436 = add nuw nsw i64 %432, 18432
  %437 = add nuw nsw i64 %432, 23040
  %438 = add nuw nsw i64 %432, 27648
  %439 = add nuw nsw i64 %432, 32256
  %440 = add nuw nsw i64 %432, 36864
  %441 = add nuw nsw i64 %432, 41472
  %442 = add nuw nsw i64 %432, 46080
  %443 = add nuw nsw i64 %432, 50688
  %444 = add nuw nsw i64 %432, 55296
  %445 = add nuw nsw i64 %432, 59904
  %446 = add nuw nsw i64 %432, 64512
  %447 = add nuw nsw i64 %432, 69120
  %448 = getelementptr inbounds float, float* %13, i64 %432
  %449 = load float, float* %448, align 4, !tbaa !173
  %450 = insertelement <16 x float> undef, float %449, i32 0
  %451 = getelementptr inbounds float, float* %13, i64 %433
  %452 = load float, float* %451, align 4, !tbaa !173
  %453 = insertelement <16 x float> %450, float %452, i32 1
  %454 = getelementptr inbounds float, float* %13, i64 %434
  %455 = load float, float* %454, align 4, !tbaa !173
  %456 = insertelement <16 x float> %453, float %455, i32 2
  %457 = getelementptr inbounds float, float* %13, i64 %435
  %458 = load float, float* %457, align 4, !tbaa !173
  %459 = insertelement <16 x float> %456, float %458, i32 3
  %460 = getelementptr inbounds float, float* %13, i64 %436
  %461 = load float, float* %460, align 4, !tbaa !173
  %462 = insertelement <16 x float> %459, float %461, i32 4
  %463 = getelementptr inbounds float, float* %13, i64 %437
  %464 = load float, float* %463, align 4, !tbaa !173
  %465 = insertelement <16 x float> %462, float %464, i32 5
  %466 = getelementptr inbounds float, float* %13, i64 %438
  %467 = load float, float* %466, align 4, !tbaa !173
  %468 = insertelement <16 x float> %465, float %467, i32 6
  %469 = getelementptr inbounds float, float* %13, i64 %439
  %470 = load float, float* %469, align 4, !tbaa !173
  %471 = insertelement <16 x float> %468, float %470, i32 7
  %472 = getelementptr inbounds float, float* %13, i64 %440
  %473 = load float, float* %472, align 4, !tbaa !173
  %474 = insertelement <16 x float> %471, float %473, i32 8
  %475 = getelementptr inbounds float, float* %13, i64 %441
  %476 = load float, float* %475, align 4, !tbaa !173
  %477 = insertelement <16 x float> %474, float %476, i32 9
  %478 = getelementptr inbounds float, float* %13, i64 %442
  %479 = load float, float* %478, align 4, !tbaa !173
  %480 = insertelement <16 x float> %477, float %479, i32 10
  %481 = getelementptr inbounds float, float* %13, i64 %443
  %482 = load float, float* %481, align 4, !tbaa !173
  %483 = insertelement <16 x float> %480, float %482, i32 11
  %484 = getelementptr inbounds float, float* %13, i64 %444
  %485 = load float, float* %484, align 4, !tbaa !173
  %486 = insertelement <16 x float> %483, float %485, i32 12
  %487 = getelementptr inbounds float, float* %13, i64 %445
  %488 = load float, float* %487, align 4, !tbaa !173
  %489 = insertelement <16 x float> %486, float %488, i32 13
  %490 = getelementptr inbounds float, float* %13, i64 %446
  %491 = load float, float* %490, align 4, !tbaa !173
  %492 = insertelement <16 x float> %489, float %491, i32 14
  %493 = getelementptr inbounds float, float* %13, i64 %447
  %494 = load float, float* %493, align 4, !tbaa !173
  %495 = insertelement <16 x float> %492, float %494, i32 15
  %496 = getelementptr inbounds [147456 x <16 x float>], [147456 x <16 x float>]* %4, i64 0, i64 0, i64 %430
  %497 = bitcast float* %496 to <16 x float>*
  store <16 x float> %495, <16 x float>* %497, align 16, !tbaa !176
  %indvars.iv.next84.2 = add nuw nsw i64 %indvars.iv83.2, 1
  %exitcond85.2 = icmp eq i64 %indvars.iv.next84.2, 512
  br i1 %exitcond85.2, label %for_end15.2, label %for_body14.2, !prof !34

for_end15.2:                                      ; preds = %for_body14.2
  %indvars.iv.next90 = add nuw nsw i64 %indvars.iv89, 1
  %exitcond91 = icmp eq i64 %indvars.iv.next90, 96
  br i1 %exitcond91, label %for_begin16.preheader, label %for_begin10.preheader, !prof !34

if_end.us.us.15:                                  ; preds = %for_begin1.preheader, %if_end.us.us.15
  %indvars.iv104 = phi i64 [ %indvars.iv.next105, %if_end.us.us.15 ], [ 0, %for_begin1.preheader ]
  %498 = shl i64 %indvars.iv104, 4
  %499 = add nuw nsw i64 %498, %7
  %500 = mul nuw nsw i64 %indvars.iv104, 196
  %501 = add nsw i64 %12, %500
  %502 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %499
  store float 0.000000e+00, float* %502, align 16, !tbaa !189
  %503 = or i64 %499, 1
  %504 = add nsw i64 %501, 1
  %505 = getelementptr inbounds float, float* %6, i64 %504
  %506 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %503
  %507 = bitcast float* %505 to <4 x i32>*
  %508 = load <4 x i32>, <4 x i32>* %507, align 4, !tbaa !196
  %509 = bitcast float* %506 to <4 x i32>*
  store <4 x i32> %508, <4 x i32>* %509, align 4, !tbaa !189
  %510 = or i64 %499, 5
  %511 = add nsw i64 %501, 5
  %512 = getelementptr inbounds float, float* %6, i64 %511
  %513 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %510
  %514 = bitcast float* %512 to <4 x i32>*
  %515 = load <4 x i32>, <4 x i32>* %514, align 4, !tbaa !196
  %516 = bitcast float* %513 to <4 x i32>*
  store <4 x i32> %515, <4 x i32>* %516, align 4, !tbaa !189
  %517 = or i64 %499, 9
  %518 = add nsw i64 %501, 9
  %519 = getelementptr inbounds float, float* %6, i64 %518
  %520 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %517
  %521 = bitcast float* %519 to <4 x i32>*
  %522 = load <4 x i32>, <4 x i32>* %521, align 4, !tbaa !196
  %523 = bitcast float* %520 to <4 x i32>*
  store <4 x i32> %522, <4 x i32>* %523, align 4, !tbaa !189
  %524 = or i64 %499, 13
  %525 = add nsw i64 %501, 13
  %526 = getelementptr inbounds float, float* %6, i64 %525
  %527 = bitcast float* %526 to i32*
  %528 = load i32, i32* %527, align 4, !tbaa !196
  %529 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %524
  %530 = bitcast float* %529 to i32*
  store i32 %528, i32* %530, align 4, !tbaa !189
  %531 = or i64 %499, 14
  %532 = add nsw i64 %501, 14
  %533 = getelementptr inbounds float, float* %6, i64 %532
  %534 = bitcast float* %533 to i32*
  %535 = load i32, i32* %534, align 4, !tbaa !196
  %536 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %531
  %537 = bitcast float* %536 to i32*
  store i32 %535, i32* %537, align 8, !tbaa !189
  %538 = or i64 %499, 15
  %539 = getelementptr inbounds [131072 x float], [131072 x float]* %5, i64 0, i64 %538
  store float 0.000000e+00, float* %539, align 4, !tbaa !189
  %indvars.iv.next105 = add nuw nsw i64 %indvars.iv104, 1
  %exitcond106 = icmp eq i64 %indvars.iv.next105, 512
  br i1 %exitcond106, label %for_end3, label %if_end.us.us.15, !prof !34
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
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 4 %0, i8* align 4 %1, i64 16384, i1 false)
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
  %5 = load float, float* %4, align 64, !tbaa !199
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
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !213
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !213
  %11 = fmul <4 x float> %wide.load, %broadcast.splat4
  %12 = fmul <4 x float> %wide.load2, %broadcast.splat6
  %13 = getelementptr inbounds float, float* %6, i64 %index
  %14 = bitcast float* %13 to <4 x float>*
  store <4 x float> %11, <4 x float>* %14, align 4, !tbaa !216
  %15 = getelementptr inbounds float, float* %13, i64 4
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %12, <4 x float>* %16, align 4, !tbaa !216
  %index.next = add i64 %index, 8
  %17 = icmp eq i64 %index.next, 4096
  br i1 %17, label %for_end, label %vector.body, !llvm.loop !219

for_end:                                          ; preds = %vector.body
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
  %10 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !220
  %11 = fcmp ogt <4 x float> %10, zeroinitializer
  %12 = select <4 x i1> %11, <4 x float> %10, <4 x float> zeroinitializer
  %13 = bitcast float* %8 to <4 x float>*
  store <4 x float> %12, <4 x float>* %13, align 4, !tbaa !223
  %14 = add nuw nsw i64 %6, 4
  %15 = getelementptr inbounds float, float* %2, i64 %14
  %16 = getelementptr inbounds float, float* %3, i64 %14
  %17 = bitcast float* %15 to <4 x float>*
  %18 = load <4 x float>, <4 x float>* %17, align 4, !tbaa !220
  %19 = fcmp ogt <4 x float> %18, zeroinitializer
  %20 = select <4 x i1> %19, <4 x float> %18, <4 x float> zeroinitializer
  %21 = bitcast float* %16 to <4 x float>*
  store <4 x float> %20, <4 x float>* %21, align 4, !tbaa !223
  %22 = add nuw nsw i64 %6, 8
  %23 = getelementptr inbounds float, float* %2, i64 %22
  %24 = getelementptr inbounds float, float* %3, i64 %22
  %25 = bitcast float* %23 to <4 x float>*
  %26 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !220
  %27 = fcmp ogt <4 x float> %26, zeroinitializer
  %28 = select <4 x i1> %27, <4 x float> %26, <4 x float> zeroinitializer
  %29 = bitcast float* %24 to <4 x float>*
  store <4 x float> %28, <4 x float>* %29, align 4, !tbaa !223
  %30 = add nuw nsw i64 %6, 12
  %31 = getelementptr inbounds float, float* %2, i64 %30
  %32 = getelementptr inbounds float, float* %3, i64 %30
  %33 = bitcast float* %31 to <4 x float>*
  %34 = load <4 x float>, <4 x float>* %33, align 4, !tbaa !220
  %35 = fcmp ogt <4 x float> %34, zeroinitializer
  %36 = select <4 x i1> %35, <4 x float> %34, <4 x float> zeroinitializer
  %37 = bitcast float* %32 to <4 x float>*
  store <4 x float> %36, <4 x float>* %37, align 4, !tbaa !223
  %38 = add nuw nsw i64 %6, 16
  %39 = getelementptr inbounds float, float* %2, i64 %38
  %40 = getelementptr inbounds float, float* %3, i64 %38
  %41 = bitcast float* %39 to <4 x float>*
  %42 = load <4 x float>, <4 x float>* %41, align 4, !tbaa !220
  %43 = fcmp ogt <4 x float> %42, zeroinitializer
  %44 = select <4 x i1> %43, <4 x float> %42, <4 x float> zeroinitializer
  %45 = bitcast float* %40 to <4 x float>*
  store <4 x float> %44, <4 x float>* %45, align 4, !tbaa !223
  %46 = add nuw nsw i64 %6, 20
  %47 = getelementptr inbounds float, float* %2, i64 %46
  %48 = getelementptr inbounds float, float* %3, i64 %46
  %49 = bitcast float* %47 to <4 x float>*
  %50 = load <4 x float>, <4 x float>* %49, align 4, !tbaa !220
  %51 = fcmp ogt <4 x float> %50, zeroinitializer
  %52 = select <4 x i1> %51, <4 x float> %50, <4 x float> zeroinitializer
  %53 = bitcast float* %48 to <4 x float>*
  store <4 x float> %52, <4 x float>* %53, align 4, !tbaa !223
  %54 = add nuw nsw i64 %6, 24
  %55 = getelementptr inbounds float, float* %2, i64 %54
  %56 = getelementptr inbounds float, float* %3, i64 %54
  %57 = bitcast float* %55 to <4 x float>*
  %58 = load <4 x float>, <4 x float>* %57, align 4, !tbaa !220
  %59 = fcmp ogt <4 x float> %58, zeroinitializer
  %60 = select <4 x i1> %59, <4 x float> %58, <4 x float> zeroinitializer
  %61 = bitcast float* %56 to <4 x float>*
  store <4 x float> %60, <4 x float>* %61, align 4, !tbaa !223
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 28
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 512
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !34
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
  %3 = alloca [14 x <16 x float>], align 64
  %4 = alloca [28 x <16 x float>], align 16
  %5 = alloca [147456 x <16 x float>], align 16
  %6 = alloca [460800 x float], align 16
  %7 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar104 = phi i64 [ 0, %entry ], [ %indvar.next105, %for_end3 ]
  %8 = mul nuw nsw i64 %indvar104, 15360
  %9 = trunc i64 %indvar104 to i32
  %10 = add i32 %9, -1
  %11 = icmp ult i32 %10, 28
  %12 = mul nuw nsw i64 %indvar104, 28
  %13 = add nsw i64 %12, -29
  br i1 %11, label %if_end.us.us.29, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep111 = getelementptr [460800 x float], [460800 x float]* %6, i64 0, i64 %8
  %scevgep111112 = bitcast float* %scevgep111 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep111112, i8 0, i64 61440, i1 false)
  br label %for_end3

for_begin7.preheader:                             ; preds = %for_end3
  %14 = bitcast [14 x <16 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0
  %15 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_end3:                                         ; preds = %if_end.us.us.29, %for_begin4.preheader.preheader
  %indvar.next105 = add nuw nsw i64 %indvar104, 1
  %exitcond120 = icmp eq i64 %indvar.next105, 30
  br i1 %exitcond120, label %for_begin7.preheader, label %for_begin1.preheader, !prof !34

for_begin10.preheader:                            ; preds = %for_end15.2, %for_begin7.preheader
  %indvars.iv98 = phi i64 [ 0, %for_begin7.preheader ], [ %indvars.iv.next99, %for_end15.2 ]
  %16 = mul nuw nsw i64 %indvars.iv98, 24576
  %17 = trunc i64 %indvars.iv98 to i32
  %18 = urem i32 %17, 3
  %19 = mul nuw nsw i32 %18, 3
  %20 = udiv i32 %17, 3
  %21 = mul nsw i32 %20, 73728
  %22 = or i32 %19, %21
  %23 = zext i32 %22 to i64
  br label %for_body14

for_begin16.preheader:                            ; preds = %for_end15.2
  %24 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 16
  %25 = bitcast float* %24 to <16 x float>*
  %26 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 32
  %27 = bitcast float* %26 to <16 x float>*
  %28 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 48
  %29 = bitcast float* %28 to <16 x float>*
  %30 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 64
  %31 = bitcast float* %30 to <16 x float>*
  %32 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 80
  %33 = bitcast float* %32 to <16 x float>*
  %34 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 96
  %35 = bitcast float* %34 to <16 x float>*
  %36 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 112
  %37 = bitcast float* %36 to <16 x float>*
  %38 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 128
  %39 = bitcast float* %38 to <16 x float>*
  %40 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 144
  %41 = bitcast float* %40 to <16 x float>*
  %42 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 160
  %43 = bitcast float* %42 to <16 x float>*
  %44 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 176
  %45 = bitcast float* %44 to <16 x float>*
  %46 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 192
  %47 = bitcast float* %46 to <16 x float>*
  %48 = getelementptr inbounds [14 x <16 x float>], [14 x <16 x float>]* %3, i64 0, i64 0, i64 208
  %49 = bitcast float* %48 to <16 x float>*
  %50 = bitcast i8* %2 to float*
  %51 = bitcast [14 x <16 x float>]* %3 to i8*
  br label %for_begin19.preheader

for_body14:                                       ; preds = %for_body14, %for_begin10.preheader
  %indvars.iv92 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next93, %for_body14 ]
  %52 = shl i64 %indvars.iv92, 4
  %53 = add nuw nsw i64 %16, %52
  %54 = mul nuw nsw i64 %indvars.iv92, 9
  %55 = add nuw nsw i64 %54, %23
  %56 = add nuw nsw i64 %55, 4608
  %57 = add nuw nsw i64 %55, 9216
  %58 = add nuw nsw i64 %55, 13824
  %59 = add nuw nsw i64 %55, 18432
  %60 = add nuw nsw i64 %55, 23040
  %61 = add nuw nsw i64 %55, 27648
  %62 = add nuw nsw i64 %55, 32256
  %63 = add nuw nsw i64 %55, 36864
  %64 = add nuw nsw i64 %55, 41472
  %65 = add nuw nsw i64 %55, 46080
  %66 = add nuw nsw i64 %55, 50688
  %67 = add nuw nsw i64 %55, 55296
  %68 = add nuw nsw i64 %55, 59904
  %69 = add nuw nsw i64 %55, 64512
  %70 = add nuw nsw i64 %55, 69120
  %71 = getelementptr inbounds float, float* %15, i64 %55
  %72 = load float, float* %71, align 4, !tbaa !226
  %73 = insertelement <16 x float> undef, float %72, i32 0
  %74 = getelementptr inbounds float, float* %15, i64 %56
  %75 = load float, float* %74, align 4, !tbaa !226
  %76 = insertelement <16 x float> %73, float %75, i32 1
  %77 = getelementptr inbounds float, float* %15, i64 %57
  %78 = load float, float* %77, align 4, !tbaa !226
  %79 = insertelement <16 x float> %76, float %78, i32 2
  %80 = getelementptr inbounds float, float* %15, i64 %58
  %81 = load float, float* %80, align 4, !tbaa !226
  %82 = insertelement <16 x float> %79, float %81, i32 3
  %83 = getelementptr inbounds float, float* %15, i64 %59
  %84 = load float, float* %83, align 4, !tbaa !226
  %85 = insertelement <16 x float> %82, float %84, i32 4
  %86 = getelementptr inbounds float, float* %15, i64 %60
  %87 = load float, float* %86, align 4, !tbaa !226
  %88 = insertelement <16 x float> %85, float %87, i32 5
  %89 = getelementptr inbounds float, float* %15, i64 %61
  %90 = load float, float* %89, align 4, !tbaa !226
  %91 = insertelement <16 x float> %88, float %90, i32 6
  %92 = getelementptr inbounds float, float* %15, i64 %62
  %93 = load float, float* %92, align 4, !tbaa !226
  %94 = insertelement <16 x float> %91, float %93, i32 7
  %95 = getelementptr inbounds float, float* %15, i64 %63
  %96 = load float, float* %95, align 4, !tbaa !226
  %97 = insertelement <16 x float> %94, float %96, i32 8
  %98 = getelementptr inbounds float, float* %15, i64 %64
  %99 = load float, float* %98, align 4, !tbaa !226
  %100 = insertelement <16 x float> %97, float %99, i32 9
  %101 = getelementptr inbounds float, float* %15, i64 %65
  %102 = load float, float* %101, align 4, !tbaa !226
  %103 = insertelement <16 x float> %100, float %102, i32 10
  %104 = getelementptr inbounds float, float* %15, i64 %66
  %105 = load float, float* %104, align 4, !tbaa !226
  %106 = insertelement <16 x float> %103, float %105, i32 11
  %107 = getelementptr inbounds float, float* %15, i64 %67
  %108 = load float, float* %107, align 4, !tbaa !226
  %109 = insertelement <16 x float> %106, float %108, i32 12
  %110 = getelementptr inbounds float, float* %15, i64 %68
  %111 = load float, float* %110, align 4, !tbaa !226
  %112 = insertelement <16 x float> %109, float %111, i32 13
  %113 = getelementptr inbounds float, float* %15, i64 %69
  %114 = load float, float* %113, align 4, !tbaa !226
  %115 = insertelement <16 x float> %112, float %114, i32 14
  %116 = getelementptr inbounds float, float* %15, i64 %70
  %117 = load float, float* %116, align 4, !tbaa !226
  %118 = insertelement <16 x float> %115, float %117, i32 15
  %119 = getelementptr inbounds [147456 x <16 x float>], [147456 x <16 x float>]* %5, i64 0, i64 0, i64 %53
  %120 = bitcast float* %119 to <16 x float>*
  store <16 x float> %118, <16 x float>* %120, align 16, !tbaa !229
  %indvars.iv.next93 = add nuw nsw i64 %indvars.iv92, 1
  %exitcond94 = icmp eq i64 %indvars.iv.next93, 512
  br i1 %exitcond94, label %for_end15, label %for_body14, !prof !34

for_end15:                                        ; preds = %for_body14
  %121 = add nuw nsw i64 %16, 8192
  %122 = add nuw nsw i64 %23, 1
  br label %for_body14.1

for_begin19.preheader:                            ; preds = %for_end36.1, %for_begin16.preheader
  %123 = phi i32 [ 0, %for_begin16.preheader ], [ %405, %for_end36.1 ]
  %124 = urem i32 %123, 28
  %125 = udiv i32 %123, 28
  %126 = mul nsw i32 %125, 73728
  %127 = zext i32 %126 to i64
  br label %for_body20

for_end18:                                        ; preds = %for_end36.1
  ret void

for_begin31.preheader:                            ; preds = %for_begin28.preheader
  %128 = mul nuw nsw i32 %124, 28
  %129 = mul nsw i32 %125, 12544
  %130 = add nuw nsw i32 %129, %128
  %131 = zext i32 %130 to i64
  br label %for_body35

for_body20:                                       ; preds = %for_begin28.preheader, %for_begin19.preheader
  %indvar = phi i64 [ 0, %for_begin19.preheader ], [ %indvar.next, %for_begin28.preheader ]
  %132 = phi i32 [ 0, %for_begin19.preheader ], [ %135, %for_begin28.preheader ]
  %133 = mul nuw nsw i64 %indvar, 14
  %scevgep = getelementptr [28 x <16 x float>], [28 x <16 x float>]* %4, i64 0, i64 %133
  %scevgep83 = bitcast <16 x float>* %scevgep to i8*
  %134 = mul nuw nsw i32 %132, 14
  call void @llvm.memset.p0i8.i64(i8* nonnull align 64 %51, i8 0, i64 896, i1 false)
  br label %for_begin25.preheader

for_begin28.preheader:                            ; preds = %for_end27
  store <16 x float> %278, <16 x float>* %.sub, align 64, !tbaa !232
  store <16 x float> %279, <16 x float>* %25, align 64, !tbaa !232
  store <16 x float> %280, <16 x float>* %27, align 64, !tbaa !232
  store <16 x float> %281, <16 x float>* %29, align 64, !tbaa !232
  store <16 x float> %282, <16 x float>* %31, align 64, !tbaa !232
  store <16 x float> %283, <16 x float>* %33, align 64, !tbaa !232
  store <16 x float> %284, <16 x float>* %35, align 64, !tbaa !232
  store <16 x float> %285, <16 x float>* %37, align 64, !tbaa !232
  store <16 x float> %286, <16 x float>* %39, align 64, !tbaa !232
  store <16 x float> %287, <16 x float>* %41, align 64, !tbaa !232
  store <16 x float> %288, <16 x float>* %43, align 64, !tbaa !232
  store <16 x float> %289, <16 x float>* %45, align 64, !tbaa !232
  store <16 x float> %290, <16 x float>* %47, align 64, !tbaa !232
  store <16 x float> %296, <16 x float>* %49, align 64, !tbaa !232
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep83, i8* nonnull align 64 %14, i64 896, i1 false)
  %135 = add nuw nsw i32 %132, 1
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond84 = icmp eq i64 %indvar.next, 2
  br i1 %exitcond84, label %for_begin31.preheader, label %for_body20, !prof !34

for_begin25.preheader:                            ; preds = %for_end27, %for_body20
  %indvars.iv77 = phi i64 [ 0, %for_body20 ], [ %indvars.iv.next78, %for_end27 ]
  %.lcssa2855 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %296, %for_end27 ]
  %.lcssa2653 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %290, %for_end27 ]
  %.lcssa2451 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %289, %for_end27 ]
  %.lcssa2249 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %288, %for_end27 ]
  %.lcssa2047 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %287, %for_end27 ]
  %.lcssa1845 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %286, %for_end27 ]
  %.lcssa1643 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %285, %for_end27 ]
  %.lcssa1441 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %284, %for_end27 ]
  %.lcssa1239 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %283, %for_end27 ]
  %.lcssa1037 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %282, %for_end27 ]
  %.lcssa835 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %281, %for_end27 ]
  %.lcssa633 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %280, %for_end27 ]
  %.lcssa432 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %279, %for_end27 ]
  %.lcssa30 = phi <16 x float> [ zeroinitializer, %for_body20 ], [ %278, %for_end27 ]
  %136 = phi i32 [ 0, %for_body20 ], [ %298, %for_end27 ]
  %137 = add nuw nsw i32 %136, %124
  %138 = mul i32 %137, 15360
  %139 = add nsw i32 %138, %134
  %140 = mul nuw nsw i64 %indvars.iv77, 24576
  %141 = add nuw nsw i64 %140, %127
  %142 = sext i32 %139 to i64
  br label %for_body26

for_body26:                                       ; preds = %for_body26, %for_begin25.preheader
  %indvars.iv = phi i64 [ 0, %for_begin25.preheader ], [ %indvars.iv.next, %for_body26 ]
  %143 = phi <16 x float> [ %.lcssa2855, %for_begin25.preheader ], [ %296, %for_body26 ]
  %144 = phi <16 x float> [ %.lcssa2653, %for_begin25.preheader ], [ %290, %for_body26 ]
  %145 = phi <16 x float> [ %.lcssa2451, %for_begin25.preheader ], [ %289, %for_body26 ]
  %146 = phi <16 x float> [ %.lcssa2249, %for_begin25.preheader ], [ %288, %for_body26 ]
  %147 = phi <16 x float> [ %.lcssa2047, %for_begin25.preheader ], [ %287, %for_body26 ]
  %148 = phi <16 x float> [ %.lcssa1845, %for_begin25.preheader ], [ %286, %for_body26 ]
  %149 = phi <16 x float> [ %.lcssa1643, %for_begin25.preheader ], [ %285, %for_body26 ]
  %150 = phi <16 x float> [ %.lcssa1441, %for_begin25.preheader ], [ %284, %for_body26 ]
  %151 = phi <16 x float> [ %.lcssa1239, %for_begin25.preheader ], [ %283, %for_body26 ]
  %152 = phi <16 x float> [ %.lcssa1037, %for_begin25.preheader ], [ %282, %for_body26 ]
  %153 = phi <16 x float> [ %.lcssa835, %for_begin25.preheader ], [ %281, %for_body26 ]
  %154 = phi <16 x float> [ %.lcssa633, %for_begin25.preheader ], [ %280, %for_body26 ]
  %155 = phi <16 x float> [ %.lcssa432, %for_begin25.preheader ], [ %279, %for_body26 ]
  %156 = phi <16 x float> [ %.lcssa30, %for_begin25.preheader ], [ %278, %for_body26 ]
  %157 = phi i32 [ 0, %for_begin25.preheader ], [ %297, %for_body26 ]
  %158 = mul nuw nsw i64 %indvars.iv, 30
  %159 = mul nuw nsw i32 %157, 30
  %160 = add nsw i64 %158, %142
  %161 = add nsw i32 %139, %159
  %162 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %160
  %163 = load float, float* %162, align 8, !tbaa !242
  %164 = insertelement <16 x float> undef, float %163, i32 0
  %165 = shufflevector <16 x float> %164, <16 x float> undef, <16 x i32> zeroinitializer
  %166 = shl nsw i64 %indvars.iv, 4
  %167 = add nuw nsw i64 %141, %166
  %168 = getelementptr inbounds [147456 x <16 x float>], [147456 x <16 x float>]* %5, i64 0, i64 0, i64 %167
  %169 = bitcast float* %168 to <16 x float>*
  %170 = load <16 x float>, <16 x float>* %169, align 16, !tbaa !229
  %171 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %165, <16 x float> %170, <16 x float> %156)
  %172 = or i32 %161, 1
  %173 = sext i32 %172 to i64
  %174 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %173
  %175 = load float, float* %174, align 4, !tbaa !242
  %176 = insertelement <16 x float> undef, float %175, i32 0
  %177 = shufflevector <16 x float> %176, <16 x float> undef, <16 x i32> zeroinitializer
  %178 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %177, <16 x float> %170, <16 x float> %155)
  %179 = add nsw i64 %160, 2
  %180 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %179
  %181 = load float, float* %180, align 8, !tbaa !242
  %182 = insertelement <16 x float> undef, float %181, i32 0
  %183 = shufflevector <16 x float> %182, <16 x float> undef, <16 x i32> zeroinitializer
  %184 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %183, <16 x float> %170, <16 x float> %154)
  %185 = add nsw i64 %160, 3
  %186 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %185
  %187 = load float, float* %186, align 4, !tbaa !242
  %188 = insertelement <16 x float> undef, float %187, i32 0
  %189 = shufflevector <16 x float> %188, <16 x float> undef, <16 x i32> zeroinitializer
  %190 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %189, <16 x float> %170, <16 x float> %153)
  %191 = add nsw i64 %160, 4
  %192 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %191
  %193 = load float, float* %192, align 8, !tbaa !242
  %194 = insertelement <16 x float> undef, float %193, i32 0
  %195 = shufflevector <16 x float> %194, <16 x float> undef, <16 x i32> zeroinitializer
  %196 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %195, <16 x float> %170, <16 x float> %152)
  %197 = add nsw i64 %160, 5
  %198 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %197
  %199 = load float, float* %198, align 4, !tbaa !242
  %200 = insertelement <16 x float> undef, float %199, i32 0
  %201 = shufflevector <16 x float> %200, <16 x float> undef, <16 x i32> zeroinitializer
  %202 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %201, <16 x float> %170, <16 x float> %151)
  %203 = add nsw i64 %160, 6
  %204 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %203
  %205 = load float, float* %204, align 8, !tbaa !242
  %206 = insertelement <16 x float> undef, float %205, i32 0
  %207 = shufflevector <16 x float> %206, <16 x float> undef, <16 x i32> zeroinitializer
  %208 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %207, <16 x float> %170, <16 x float> %150)
  %209 = add nsw i64 %160, 7
  %210 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %209
  %211 = load float, float* %210, align 4, !tbaa !242
  %212 = insertelement <16 x float> undef, float %211, i32 0
  %213 = shufflevector <16 x float> %212, <16 x float> undef, <16 x i32> zeroinitializer
  %214 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %213, <16 x float> %170, <16 x float> %149)
  %215 = add nsw i64 %160, 8
  %216 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %215
  %217 = load float, float* %216, align 8, !tbaa !242
  %218 = insertelement <16 x float> undef, float %217, i32 0
  %219 = shufflevector <16 x float> %218, <16 x float> undef, <16 x i32> zeroinitializer
  %220 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %219, <16 x float> %170, <16 x float> %148)
  %221 = add nsw i64 %160, 9
  %222 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %221
  %223 = load float, float* %222, align 4, !tbaa !242
  %224 = insertelement <16 x float> undef, float %223, i32 0
  %225 = shufflevector <16 x float> %224, <16 x float> undef, <16 x i32> zeroinitializer
  %226 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %225, <16 x float> %170, <16 x float> %147)
  %227 = add nsw i64 %160, 10
  %228 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %227
  %229 = load float, float* %228, align 8, !tbaa !242
  %230 = insertelement <16 x float> undef, float %229, i32 0
  %231 = shufflevector <16 x float> %230, <16 x float> undef, <16 x i32> zeroinitializer
  %232 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %231, <16 x float> %170, <16 x float> %146)
  %233 = add nsw i64 %160, 11
  %234 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %233
  %235 = load float, float* %234, align 4, !tbaa !242
  %236 = insertelement <16 x float> undef, float %235, i32 0
  %237 = shufflevector <16 x float> %236, <16 x float> undef, <16 x i32> zeroinitializer
  %238 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %237, <16 x float> %170, <16 x float> %145)
  %239 = add nsw i64 %160, 12
  %240 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %239
  %241 = load float, float* %240, align 8, !tbaa !242
  %242 = insertelement <16 x float> undef, float %241, i32 0
  %243 = shufflevector <16 x float> %242, <16 x float> undef, <16 x i32> zeroinitializer
  %244 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %243, <16 x float> %170, <16 x float> %144)
  %245 = add nsw i64 %160, 13
  %246 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %245
  %247 = load float, float* %246, align 4, !tbaa !242
  %248 = insertelement <16 x float> undef, float %247, i32 0
  %249 = shufflevector <16 x float> %248, <16 x float> undef, <16 x i32> zeroinitializer
  %250 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %249, <16 x float> %170, <16 x float> %143)
  %251 = add nuw nsw i64 %167, 8192
  %252 = getelementptr inbounds [147456 x <16 x float>], [147456 x <16 x float>]* %5, i64 0, i64 0, i64 %251
  %253 = bitcast float* %252 to <16 x float>*
  %254 = load <16 x float>, <16 x float>* %253, align 16, !tbaa !229
  %255 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %177, <16 x float> %254, <16 x float> %171)
  %256 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %183, <16 x float> %254, <16 x float> %178)
  %257 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %189, <16 x float> %254, <16 x float> %184)
  %258 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %195, <16 x float> %254, <16 x float> %190)
  %259 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %201, <16 x float> %254, <16 x float> %196)
  %260 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %207, <16 x float> %254, <16 x float> %202)
  %261 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %213, <16 x float> %254, <16 x float> %208)
  %262 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %219, <16 x float> %254, <16 x float> %214)
  %263 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %225, <16 x float> %254, <16 x float> %220)
  %264 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %231, <16 x float> %254, <16 x float> %226)
  %265 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %237, <16 x float> %254, <16 x float> %232)
  %266 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %243, <16 x float> %254, <16 x float> %238)
  %267 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %249, <16 x float> %254, <16 x float> %244)
  %268 = add nsw i64 %160, 14
  %269 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %268
  %270 = load float, float* %269, align 8, !tbaa !242
  %271 = insertelement <16 x float> undef, float %270, i32 0
  %272 = shufflevector <16 x float> %271, <16 x float> undef, <16 x i32> zeroinitializer
  %273 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %272, <16 x float> %254, <16 x float> %250)
  %274 = add nuw nsw i64 %167, 16384
  %275 = getelementptr inbounds [147456 x <16 x float>], [147456 x <16 x float>]* %5, i64 0, i64 0, i64 %274
  %276 = bitcast float* %275 to <16 x float>*
  %277 = load <16 x float>, <16 x float>* %276, align 16, !tbaa !229
  %278 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %183, <16 x float> %277, <16 x float> %255)
  %279 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %189, <16 x float> %277, <16 x float> %256)
  %280 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %195, <16 x float> %277, <16 x float> %257)
  %281 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %201, <16 x float> %277, <16 x float> %258)
  %282 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %207, <16 x float> %277, <16 x float> %259)
  %283 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %213, <16 x float> %277, <16 x float> %260)
  %284 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %219, <16 x float> %277, <16 x float> %261)
  %285 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %225, <16 x float> %277, <16 x float> %262)
  %286 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %231, <16 x float> %277, <16 x float> %263)
  %287 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %237, <16 x float> %277, <16 x float> %264)
  %288 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %243, <16 x float> %277, <16 x float> %265)
  %289 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %249, <16 x float> %277, <16 x float> %266)
  %290 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %272, <16 x float> %277, <16 x float> %267)
  %291 = add nsw i64 %160, 15
  %292 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %291
  %293 = load float, float* %292, align 4, !tbaa !242
  %294 = insertelement <16 x float> undef, float %293, i32 0
  %295 = shufflevector <16 x float> %294, <16 x float> undef, <16 x i32> zeroinitializer
  %296 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %295, <16 x float> %277, <16 x float> %273)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %297 = add nuw nsw i32 %157, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 512
  br i1 %exitcond, label %for_end27, label %for_body26, !prof !34

for_end27:                                        ; preds = %for_body26
  %indvars.iv.next78 = add nuw nsw i64 %indvars.iv77, 1
  %298 = add nuw nsw i32 %136, 1
  %exitcond79 = icmp eq i64 %indvars.iv.next78, 3
  br i1 %exitcond79, label %for_begin28.preheader, label %for_begin25.preheader, !prof !34

for_body35:                                       ; preds = %for_body35, %for_begin31.preheader
  %indvars.iv85 = phi i64 [ 0, %for_begin31.preheader ], [ %indvars.iv.next86, %for_body35 ]
  %299 = add nuw nsw i64 %indvars.iv85, %131
  %300 = add nuw nsw i64 %299, 784
  %301 = add nuw nsw i64 %299, 1568
  %302 = add nuw nsw i64 %299, 2352
  %303 = add nuw nsw i64 %299, 3136
  %304 = add nuw nsw i64 %299, 3920
  %305 = add nuw nsw i64 %299, 4704
  %306 = add nuw nsw i64 %299, 5488
  %307 = add nuw nsw i64 %299, 6272
  %308 = add nuw nsw i64 %299, 7056
  %309 = add nuw nsw i64 %299, 7840
  %310 = add nuw nsw i64 %299, 8624
  %311 = add nuw nsw i64 %299, 9408
  %312 = add nuw nsw i64 %299, 10192
  %313 = add nuw nsw i64 %299, 10976
  %314 = add nuw nsw i64 %299, 11760
  %315 = shl i64 %indvars.iv85, 4
  %316 = getelementptr inbounds [28 x <16 x float>], [28 x <16 x float>]* %4, i64 0, i64 0, i64 %315
  %317 = bitcast float* %316 to <16 x float>*
  %318 = load <16 x float>, <16 x float>* %317, align 16, !tbaa !245
  %319 = getelementptr inbounds float, float* %50, i64 %299
  %320 = extractelement <16 x float> %318, i64 0
  store float %320, float* %319, align 4, !tbaa !248
  %321 = getelementptr inbounds float, float* %50, i64 %300
  %322 = extractelement <16 x float> %318, i64 1
  store float %322, float* %321, align 4, !tbaa !248
  %323 = getelementptr inbounds float, float* %50, i64 %301
  %324 = extractelement <16 x float> %318, i64 2
  store float %324, float* %323, align 4, !tbaa !248
  %325 = getelementptr inbounds float, float* %50, i64 %302
  %326 = extractelement <16 x float> %318, i64 3
  store float %326, float* %325, align 4, !tbaa !248
  %327 = getelementptr inbounds float, float* %50, i64 %303
  %328 = extractelement <16 x float> %318, i64 4
  store float %328, float* %327, align 4, !tbaa !248
  %329 = getelementptr inbounds float, float* %50, i64 %304
  %330 = extractelement <16 x float> %318, i64 5
  store float %330, float* %329, align 4, !tbaa !248
  %331 = getelementptr inbounds float, float* %50, i64 %305
  %332 = extractelement <16 x float> %318, i64 6
  store float %332, float* %331, align 4, !tbaa !248
  %333 = getelementptr inbounds float, float* %50, i64 %306
  %334 = extractelement <16 x float> %318, i64 7
  store float %334, float* %333, align 4, !tbaa !248
  %335 = getelementptr inbounds float, float* %50, i64 %307
  %336 = extractelement <16 x float> %318, i64 8
  store float %336, float* %335, align 4, !tbaa !248
  %337 = getelementptr inbounds float, float* %50, i64 %308
  %338 = extractelement <16 x float> %318, i64 9
  store float %338, float* %337, align 4, !tbaa !248
  %339 = getelementptr inbounds float, float* %50, i64 %309
  %340 = extractelement <16 x float> %318, i64 10
  store float %340, float* %339, align 4, !tbaa !248
  %341 = getelementptr inbounds float, float* %50, i64 %310
  %342 = extractelement <16 x float> %318, i64 11
  store float %342, float* %341, align 4, !tbaa !248
  %343 = getelementptr inbounds float, float* %50, i64 %311
  %344 = extractelement <16 x float> %318, i64 12
  store float %344, float* %343, align 4, !tbaa !248
  %345 = getelementptr inbounds float, float* %50, i64 %312
  %346 = extractelement <16 x float> %318, i64 13
  store float %346, float* %345, align 4, !tbaa !248
  %347 = getelementptr inbounds float, float* %50, i64 %313
  %348 = extractelement <16 x float> %318, i64 14
  store float %348, float* %347, align 4, !tbaa !248
  %349 = getelementptr inbounds float, float* %50, i64 %314
  %350 = extractelement <16 x float> %318, i64 15
  store float %350, float* %349, align 4, !tbaa !248
  %indvars.iv.next86 = add nuw nsw i64 %indvars.iv85, 1
  %exitcond87 = icmp eq i64 %indvars.iv.next86, 14
  br i1 %exitcond87, label %for_end36, label %for_body35, !prof !34

for_end36:                                        ; preds = %for_body35
  %351 = add nuw nsw i64 %131, 14
  br label %for_body35.1

for_body35.1:                                     ; preds = %for_body35.1, %for_end36
  %indvars.iv85.1 = phi i64 [ 0, %for_end36 ], [ %indvars.iv.next86.1, %for_body35.1 ]
  %352 = add nuw nsw i64 %351, %indvars.iv85.1
  %353 = add nuw nsw i64 %352, 784
  %354 = add nuw nsw i64 %352, 1568
  %355 = add nuw nsw i64 %352, 2352
  %356 = add nuw nsw i64 %352, 3136
  %357 = add nuw nsw i64 %352, 3920
  %358 = add nuw nsw i64 %352, 4704
  %359 = add nuw nsw i64 %352, 5488
  %360 = add nuw nsw i64 %352, 6272
  %361 = add nuw nsw i64 %352, 7056
  %362 = add nuw nsw i64 %352, 7840
  %363 = add nuw nsw i64 %352, 8624
  %364 = add nuw nsw i64 %352, 9408
  %365 = add nuw nsw i64 %352, 10192
  %366 = add nuw nsw i64 %352, 10976
  %367 = add nuw nsw i64 %352, 11760
  %368 = shl i64 %indvars.iv85.1, 4
  %369 = add nuw nsw i64 %368, 224
  %370 = getelementptr inbounds [28 x <16 x float>], [28 x <16 x float>]* %4, i64 0, i64 0, i64 %369
  %371 = bitcast float* %370 to <16 x float>*
  %372 = load <16 x float>, <16 x float>* %371, align 16, !tbaa !245
  %373 = getelementptr inbounds float, float* %50, i64 %352
  %374 = extractelement <16 x float> %372, i64 0
  store float %374, float* %373, align 4, !tbaa !248
  %375 = getelementptr inbounds float, float* %50, i64 %353
  %376 = extractelement <16 x float> %372, i64 1
  store float %376, float* %375, align 4, !tbaa !248
  %377 = getelementptr inbounds float, float* %50, i64 %354
  %378 = extractelement <16 x float> %372, i64 2
  store float %378, float* %377, align 4, !tbaa !248
  %379 = getelementptr inbounds float, float* %50, i64 %355
  %380 = extractelement <16 x float> %372, i64 3
  store float %380, float* %379, align 4, !tbaa !248
  %381 = getelementptr inbounds float, float* %50, i64 %356
  %382 = extractelement <16 x float> %372, i64 4
  store float %382, float* %381, align 4, !tbaa !248
  %383 = getelementptr inbounds float, float* %50, i64 %357
  %384 = extractelement <16 x float> %372, i64 5
  store float %384, float* %383, align 4, !tbaa !248
  %385 = getelementptr inbounds float, float* %50, i64 %358
  %386 = extractelement <16 x float> %372, i64 6
  store float %386, float* %385, align 4, !tbaa !248
  %387 = getelementptr inbounds float, float* %50, i64 %359
  %388 = extractelement <16 x float> %372, i64 7
  store float %388, float* %387, align 4, !tbaa !248
  %389 = getelementptr inbounds float, float* %50, i64 %360
  %390 = extractelement <16 x float> %372, i64 8
  store float %390, float* %389, align 4, !tbaa !248
  %391 = getelementptr inbounds float, float* %50, i64 %361
  %392 = extractelement <16 x float> %372, i64 9
  store float %392, float* %391, align 4, !tbaa !248
  %393 = getelementptr inbounds float, float* %50, i64 %362
  %394 = extractelement <16 x float> %372, i64 10
  store float %394, float* %393, align 4, !tbaa !248
  %395 = getelementptr inbounds float, float* %50, i64 %363
  %396 = extractelement <16 x float> %372, i64 11
  store float %396, float* %395, align 4, !tbaa !248
  %397 = getelementptr inbounds float, float* %50, i64 %364
  %398 = extractelement <16 x float> %372, i64 12
  store float %398, float* %397, align 4, !tbaa !248
  %399 = getelementptr inbounds float, float* %50, i64 %365
  %400 = extractelement <16 x float> %372, i64 13
  store float %400, float* %399, align 4, !tbaa !248
  %401 = getelementptr inbounds float, float* %50, i64 %366
  %402 = extractelement <16 x float> %372, i64 14
  store float %402, float* %401, align 4, !tbaa !248
  %403 = getelementptr inbounds float, float* %50, i64 %367
  %404 = extractelement <16 x float> %372, i64 15
  store float %404, float* %403, align 4, !tbaa !248
  %indvars.iv.next86.1 = add nuw nsw i64 %indvars.iv85.1, 1
  %exitcond87.1 = icmp eq i64 %indvars.iv.next86.1, 14
  br i1 %exitcond87.1, label %for_end36.1, label %for_body35.1, !prof !34

for_end36.1:                                      ; preds = %for_body35.1
  %405 = add nuw nsw i32 %123, 1
  %exitcond91 = icmp eq i32 %405, 896
  br i1 %exitcond91, label %for_end18, label %for_begin19.preheader, !prof !34

for_body14.1:                                     ; preds = %for_body14.1, %for_end15
  %indvars.iv92.1 = phi i64 [ 0, %for_end15 ], [ %indvars.iv.next93.1, %for_body14.1 ]
  %406 = shl i64 %indvars.iv92.1, 4
  %407 = add nuw nsw i64 %121, %406
  %408 = mul nuw nsw i64 %indvars.iv92.1, 9
  %409 = add nuw nsw i64 %122, %408
  %410 = add nuw nsw i64 %409, 4608
  %411 = add nuw nsw i64 %409, 9216
  %412 = add nuw nsw i64 %409, 13824
  %413 = add nuw nsw i64 %409, 18432
  %414 = add nuw nsw i64 %409, 23040
  %415 = add nuw nsw i64 %409, 27648
  %416 = add nuw nsw i64 %409, 32256
  %417 = add nuw nsw i64 %409, 36864
  %418 = add nuw nsw i64 %409, 41472
  %419 = add nuw nsw i64 %409, 46080
  %420 = add nuw nsw i64 %409, 50688
  %421 = add nuw nsw i64 %409, 55296
  %422 = add nuw nsw i64 %409, 59904
  %423 = add nuw nsw i64 %409, 64512
  %424 = add nuw nsw i64 %409, 69120
  %425 = getelementptr inbounds float, float* %15, i64 %409
  %426 = load float, float* %425, align 4, !tbaa !226
  %427 = insertelement <16 x float> undef, float %426, i32 0
  %428 = getelementptr inbounds float, float* %15, i64 %410
  %429 = load float, float* %428, align 4, !tbaa !226
  %430 = insertelement <16 x float> %427, float %429, i32 1
  %431 = getelementptr inbounds float, float* %15, i64 %411
  %432 = load float, float* %431, align 4, !tbaa !226
  %433 = insertelement <16 x float> %430, float %432, i32 2
  %434 = getelementptr inbounds float, float* %15, i64 %412
  %435 = load float, float* %434, align 4, !tbaa !226
  %436 = insertelement <16 x float> %433, float %435, i32 3
  %437 = getelementptr inbounds float, float* %15, i64 %413
  %438 = load float, float* %437, align 4, !tbaa !226
  %439 = insertelement <16 x float> %436, float %438, i32 4
  %440 = getelementptr inbounds float, float* %15, i64 %414
  %441 = load float, float* %440, align 4, !tbaa !226
  %442 = insertelement <16 x float> %439, float %441, i32 5
  %443 = getelementptr inbounds float, float* %15, i64 %415
  %444 = load float, float* %443, align 4, !tbaa !226
  %445 = insertelement <16 x float> %442, float %444, i32 6
  %446 = getelementptr inbounds float, float* %15, i64 %416
  %447 = load float, float* %446, align 4, !tbaa !226
  %448 = insertelement <16 x float> %445, float %447, i32 7
  %449 = getelementptr inbounds float, float* %15, i64 %417
  %450 = load float, float* %449, align 4, !tbaa !226
  %451 = insertelement <16 x float> %448, float %450, i32 8
  %452 = getelementptr inbounds float, float* %15, i64 %418
  %453 = load float, float* %452, align 4, !tbaa !226
  %454 = insertelement <16 x float> %451, float %453, i32 9
  %455 = getelementptr inbounds float, float* %15, i64 %419
  %456 = load float, float* %455, align 4, !tbaa !226
  %457 = insertelement <16 x float> %454, float %456, i32 10
  %458 = getelementptr inbounds float, float* %15, i64 %420
  %459 = load float, float* %458, align 4, !tbaa !226
  %460 = insertelement <16 x float> %457, float %459, i32 11
  %461 = getelementptr inbounds float, float* %15, i64 %421
  %462 = load float, float* %461, align 4, !tbaa !226
  %463 = insertelement <16 x float> %460, float %462, i32 12
  %464 = getelementptr inbounds float, float* %15, i64 %422
  %465 = load float, float* %464, align 4, !tbaa !226
  %466 = insertelement <16 x float> %463, float %465, i32 13
  %467 = getelementptr inbounds float, float* %15, i64 %423
  %468 = load float, float* %467, align 4, !tbaa !226
  %469 = insertelement <16 x float> %466, float %468, i32 14
  %470 = getelementptr inbounds float, float* %15, i64 %424
  %471 = load float, float* %470, align 4, !tbaa !226
  %472 = insertelement <16 x float> %469, float %471, i32 15
  %473 = getelementptr inbounds [147456 x <16 x float>], [147456 x <16 x float>]* %5, i64 0, i64 0, i64 %407
  %474 = bitcast float* %473 to <16 x float>*
  store <16 x float> %472, <16 x float>* %474, align 16, !tbaa !229
  %indvars.iv.next93.1 = add nuw nsw i64 %indvars.iv92.1, 1
  %exitcond94.1 = icmp eq i64 %indvars.iv.next93.1, 512
  br i1 %exitcond94.1, label %for_end15.1, label %for_body14.1, !prof !34

for_end15.1:                                      ; preds = %for_body14.1
  %475 = add nuw nsw i64 %16, 16384
  %476 = add nuw nsw i64 %23, 2
  br label %for_body14.2

for_body14.2:                                     ; preds = %for_body14.2, %for_end15.1
  %indvars.iv92.2 = phi i64 [ 0, %for_end15.1 ], [ %indvars.iv.next93.2, %for_body14.2 ]
  %477 = shl i64 %indvars.iv92.2, 4
  %478 = add nuw nsw i64 %475, %477
  %479 = mul nuw nsw i64 %indvars.iv92.2, 9
  %480 = add nuw nsw i64 %476, %479
  %481 = add nuw nsw i64 %480, 4608
  %482 = add nuw nsw i64 %480, 9216
  %483 = add nuw nsw i64 %480, 13824
  %484 = add nuw nsw i64 %480, 18432
  %485 = add nuw nsw i64 %480, 23040
  %486 = add nuw nsw i64 %480, 27648
  %487 = add nuw nsw i64 %480, 32256
  %488 = add nuw nsw i64 %480, 36864
  %489 = add nuw nsw i64 %480, 41472
  %490 = add nuw nsw i64 %480, 46080
  %491 = add nuw nsw i64 %480, 50688
  %492 = add nuw nsw i64 %480, 55296
  %493 = add nuw nsw i64 %480, 59904
  %494 = add nuw nsw i64 %480, 64512
  %495 = add nuw nsw i64 %480, 69120
  %496 = getelementptr inbounds float, float* %15, i64 %480
  %497 = load float, float* %496, align 4, !tbaa !226
  %498 = insertelement <16 x float> undef, float %497, i32 0
  %499 = getelementptr inbounds float, float* %15, i64 %481
  %500 = load float, float* %499, align 4, !tbaa !226
  %501 = insertelement <16 x float> %498, float %500, i32 1
  %502 = getelementptr inbounds float, float* %15, i64 %482
  %503 = load float, float* %502, align 4, !tbaa !226
  %504 = insertelement <16 x float> %501, float %503, i32 2
  %505 = getelementptr inbounds float, float* %15, i64 %483
  %506 = load float, float* %505, align 4, !tbaa !226
  %507 = insertelement <16 x float> %504, float %506, i32 3
  %508 = getelementptr inbounds float, float* %15, i64 %484
  %509 = load float, float* %508, align 4, !tbaa !226
  %510 = insertelement <16 x float> %507, float %509, i32 4
  %511 = getelementptr inbounds float, float* %15, i64 %485
  %512 = load float, float* %511, align 4, !tbaa !226
  %513 = insertelement <16 x float> %510, float %512, i32 5
  %514 = getelementptr inbounds float, float* %15, i64 %486
  %515 = load float, float* %514, align 4, !tbaa !226
  %516 = insertelement <16 x float> %513, float %515, i32 6
  %517 = getelementptr inbounds float, float* %15, i64 %487
  %518 = load float, float* %517, align 4, !tbaa !226
  %519 = insertelement <16 x float> %516, float %518, i32 7
  %520 = getelementptr inbounds float, float* %15, i64 %488
  %521 = load float, float* %520, align 4, !tbaa !226
  %522 = insertelement <16 x float> %519, float %521, i32 8
  %523 = getelementptr inbounds float, float* %15, i64 %489
  %524 = load float, float* %523, align 4, !tbaa !226
  %525 = insertelement <16 x float> %522, float %524, i32 9
  %526 = getelementptr inbounds float, float* %15, i64 %490
  %527 = load float, float* %526, align 4, !tbaa !226
  %528 = insertelement <16 x float> %525, float %527, i32 10
  %529 = getelementptr inbounds float, float* %15, i64 %491
  %530 = load float, float* %529, align 4, !tbaa !226
  %531 = insertelement <16 x float> %528, float %530, i32 11
  %532 = getelementptr inbounds float, float* %15, i64 %492
  %533 = load float, float* %532, align 4, !tbaa !226
  %534 = insertelement <16 x float> %531, float %533, i32 12
  %535 = getelementptr inbounds float, float* %15, i64 %493
  %536 = load float, float* %535, align 4, !tbaa !226
  %537 = insertelement <16 x float> %534, float %536, i32 13
  %538 = getelementptr inbounds float, float* %15, i64 %494
  %539 = load float, float* %538, align 4, !tbaa !226
  %540 = insertelement <16 x float> %537, float %539, i32 14
  %541 = getelementptr inbounds float, float* %15, i64 %495
  %542 = load float, float* %541, align 4, !tbaa !226
  %543 = insertelement <16 x float> %540, float %542, i32 15
  %544 = getelementptr inbounds [147456 x <16 x float>], [147456 x <16 x float>]* %5, i64 0, i64 0, i64 %478
  %545 = bitcast float* %544 to <16 x float>*
  store <16 x float> %543, <16 x float>* %545, align 16, !tbaa !229
  %indvars.iv.next93.2 = add nuw nsw i64 %indvars.iv92.2, 1
  %exitcond94.2 = icmp eq i64 %indvars.iv.next93.2, 512
  br i1 %exitcond94.2, label %for_end15.2, label %for_body14.2, !prof !34

for_end15.2:                                      ; preds = %for_body14.2
  %indvars.iv.next99 = add nuw nsw i64 %indvars.iv98, 1
  %exitcond100 = icmp eq i64 %indvars.iv.next99, 96
  br i1 %exitcond100, label %for_begin16.preheader, label %for_begin10.preheader, !prof !34

if_end.us.us.29:                                  ; preds = %for_begin1.preheader, %if_end.us.us.29
  %indvars.iv116 = phi i64 [ %indvars.iv.next117, %if_end.us.us.29 ], [ 0, %for_begin1.preheader ]
  %546 = mul nuw nsw i64 %indvars.iv116, 30
  %547 = add nuw nsw i64 %546, %8
  %548 = mul nuw nsw i64 %indvars.iv116, 784
  %549 = add nsw i64 %13, %548
  %550 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %547
  store float 0.000000e+00, float* %550, align 8, !tbaa !242
  %551 = or i64 %547, 1
  %552 = add nsw i64 %549, 1
  %553 = getelementptr inbounds float, float* %7, i64 %552
  %554 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %551
  %555 = bitcast float* %553 to <4 x i32>*
  %556 = load <4 x i32>, <4 x i32>* %555, align 4, !tbaa !251
  %557 = bitcast float* %554 to <4 x i32>*
  store <4 x i32> %556, <4 x i32>* %557, align 4, !tbaa !242
  %558 = add nuw nsw i64 %547, 5
  %559 = add nsw i64 %549, 5
  %560 = getelementptr inbounds float, float* %7, i64 %559
  %561 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %558
  %562 = bitcast float* %560 to <4 x i32>*
  %563 = load <4 x i32>, <4 x i32>* %562, align 4, !tbaa !251
  %564 = bitcast float* %561 to <4 x i32>*
  store <4 x i32> %563, <4 x i32>* %564, align 4, !tbaa !242
  %565 = add nuw nsw i64 %547, 9
  %566 = add nsw i64 %549, 9
  %567 = getelementptr inbounds float, float* %7, i64 %566
  %568 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %565
  %569 = bitcast float* %567 to <4 x i32>*
  %570 = load <4 x i32>, <4 x i32>* %569, align 4, !tbaa !251
  %571 = bitcast float* %568 to <4 x i32>*
  store <4 x i32> %570, <4 x i32>* %571, align 4, !tbaa !242
  %572 = add nuw nsw i64 %547, 13
  %573 = add nsw i64 %549, 13
  %574 = getelementptr inbounds float, float* %7, i64 %573
  %575 = bitcast float* %574 to i32*
  %576 = load i32, i32* %575, align 4, !tbaa !251
  %577 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %572
  %578 = bitcast float* %577 to i32*
  store i32 %576, i32* %578, align 4, !tbaa !242
  %579 = add nuw nsw i64 %547, 14
  %580 = add nsw i64 %549, 14
  %581 = getelementptr inbounds float, float* %7, i64 %580
  %582 = bitcast float* %581 to i32*
  %583 = load i32, i32* %582, align 4, !tbaa !251
  %584 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %579
  %585 = bitcast float* %584 to i32*
  store i32 %583, i32* %585, align 8, !tbaa !242
  %586 = add nuw nsw i64 %547, 15
  %587 = add nsw i64 %549, 15
  %588 = getelementptr inbounds float, float* %7, i64 %587
  %589 = bitcast float* %588 to i32*
  %590 = load i32, i32* %589, align 4, !tbaa !251
  %591 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %586
  %592 = bitcast float* %591 to i32*
  store i32 %590, i32* %592, align 4, !tbaa !242
  %593 = add nuw nsw i64 %547, 16
  %594 = add nsw i64 %549, 16
  %595 = getelementptr inbounds float, float* %7, i64 %594
  %596 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %593
  %597 = bitcast float* %595 to <4 x i32>*
  %598 = load <4 x i32>, <4 x i32>* %597, align 4, !tbaa !251
  %599 = bitcast float* %596 to <4 x i32>*
  store <4 x i32> %598, <4 x i32>* %599, align 8, !tbaa !242
  %600 = add nuw nsw i64 %547, 20
  %601 = add nsw i64 %549, 20
  %602 = getelementptr inbounds float, float* %7, i64 %601
  %603 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %600
  %604 = bitcast float* %602 to <4 x i32>*
  %605 = load <4 x i32>, <4 x i32>* %604, align 4, !tbaa !251
  %606 = bitcast float* %603 to <4 x i32>*
  store <4 x i32> %605, <4 x i32>* %606, align 8, !tbaa !242
  %607 = add nuw nsw i64 %547, 24
  %608 = add nsw i64 %549, 24
  %609 = getelementptr inbounds float, float* %7, i64 %608
  %610 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %607
  %611 = bitcast float* %609 to <4 x i32>*
  %612 = load <4 x i32>, <4 x i32>* %611, align 4, !tbaa !251
  %613 = bitcast float* %610 to <4 x i32>*
  store <4 x i32> %612, <4 x i32>* %613, align 8, !tbaa !242
  %614 = add nuw nsw i64 %547, 28
  %615 = add nsw i64 %549, 28
  %616 = getelementptr inbounds float, float* %7, i64 %615
  %617 = bitcast float* %616 to i32*
  %618 = load i32, i32* %617, align 4, !tbaa !251
  %619 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %614
  %620 = bitcast float* %619 to i32*
  store i32 %618, i32* %620, align 8, !tbaa !242
  %621 = add nuw nsw i64 %547, 29
  %622 = getelementptr inbounds [460800 x float], [460800 x float]* %6, i64 0, i64 %621
  store float 0.000000e+00, float* %622, align 4, !tbaa !242
  %indvars.iv.next117 = add nuw nsw i64 %indvars.iv116, 1
  %exitcond118 = icmp eq i64 %indvars.iv.next117, 512
  br i1 %exitcond118, label %for_end3, label %if_end.us.us.29, !prof !34
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
  %10 = load <16 x float>, <16 x float>* %9, align 64, !tbaa !254
  %11 = add nuw nsw i64 %7, %6
  %12 = getelementptr inbounds float, float* %4, i64 %11
  %13 = bitcast float* %12 to <16 x float>*
  %14 = load <16 x float>, <16 x float>* %13, align 64, !tbaa !257
  %15 = tail call <16 x float> @llvm.fmuladd.v16f32(<16 x float> %10, <16 x float> %14, <16 x float> %.02)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !34

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
  store float %32, float* %16, align 4, !tbaa !260
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 4096
  br i1 %exitcond5, label %for_end, label %for_begin1.preheader, !prof !34
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_bias_add_6(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_bias_add_6_compute_(i8* %16, i8* %12, i8* %14)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_bias_add_6_compute_(i8* noalias nocapture, i8* noalias nocapture readonly, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %3 = bitcast i8* %2 to float*
  %4 = bitcast i8* %1 to float*
  %5 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv4 = phi i64 [ 0, %entry ], [ %indvars.iv.next5, %for_end3 ]
  %6 = mul nuw nsw i64 %indvars.iv4, 50176
  %7 = getelementptr inbounds float, float* %3, i64 %indvars.iv4
  %8 = load float, float* %7, align 4, !tbaa !263
  %broadcast.splatinsert9 = insertelement <4 x float> undef, float %8, i32 0
  %broadcast.splat10 = shufflevector <4 x float> %broadcast.splatinsert9, <4 x float> undef, <4 x i32> zeroinitializer
  %broadcast.splatinsert11 = insertelement <4 x float> undef, float %8, i32 0
  %broadcast.splat12 = shufflevector <4 x float> %broadcast.splatinsert11, <4 x float> undef, <4 x i32> zeroinitializer
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv1 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next2, %for_begin4.preheader ]
  %9 = mul nuw nsw i64 %indvars.iv1, 224
  %10 = add nuw nsw i64 %9, %6
  %11 = getelementptr inbounds float, float* %4, i64 %10
  %12 = bitcast float* %11 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %12, align 4, !tbaa !266
  %13 = getelementptr inbounds float, float* %11, i64 4
  %14 = bitcast float* %13 to <4 x float>*
  %wide.load8 = load <4 x float>, <4 x float>* %14, align 4, !tbaa !266
  %15 = fadd <4 x float> %broadcast.splat10, %wide.load
  %16 = fadd <4 x float> %broadcast.splat12, %wide.load8
  %17 = getelementptr inbounds float, float* %5, i64 %10
  %18 = bitcast float* %17 to <4 x float>*
  store <4 x float> %15, <4 x float>* %18, align 4, !tbaa !269
  %19 = getelementptr inbounds float, float* %17, i64 4
  %20 = bitcast float* %19 to <4 x float>*
  store <4 x float> %16, <4 x float>* %20, align 4, !tbaa !269
  %21 = or i64 %10, 8
  %22 = getelementptr inbounds float, float* %4, i64 %21
  %23 = bitcast float* %22 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %23, align 4, !tbaa !266
  %24 = getelementptr inbounds float, float* %22, i64 4
  %25 = bitcast float* %24 to <4 x float>*
  %wide.load8.1 = load <4 x float>, <4 x float>* %25, align 4, !tbaa !266
  %26 = fadd <4 x float> %broadcast.splat10, %wide.load.1
  %27 = fadd <4 x float> %broadcast.splat12, %wide.load8.1
  %28 = getelementptr inbounds float, float* %5, i64 %21
  %29 = bitcast float* %28 to <4 x float>*
  store <4 x float> %26, <4 x float>* %29, align 4, !tbaa !269
  %30 = getelementptr inbounds float, float* %28, i64 4
  %31 = bitcast float* %30 to <4 x float>*
  store <4 x float> %27, <4 x float>* %31, align 4, !tbaa !269
  %32 = or i64 %10, 16
  %33 = getelementptr inbounds float, float* %4, i64 %32
  %34 = bitcast float* %33 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %34, align 4, !tbaa !266
  %35 = getelementptr inbounds float, float* %33, i64 4
  %36 = bitcast float* %35 to <4 x float>*
  %wide.load8.2 = load <4 x float>, <4 x float>* %36, align 4, !tbaa !266
  %37 = fadd <4 x float> %broadcast.splat10, %wide.load.2
  %38 = fadd <4 x float> %broadcast.splat12, %wide.load8.2
  %39 = getelementptr inbounds float, float* %5, i64 %32
  %40 = bitcast float* %39 to <4 x float>*
  store <4 x float> %37, <4 x float>* %40, align 4, !tbaa !269
  %41 = getelementptr inbounds float, float* %39, i64 4
  %42 = bitcast float* %41 to <4 x float>*
  store <4 x float> %38, <4 x float>* %42, align 4, !tbaa !269
  %43 = or i64 %10, 24
  %44 = getelementptr inbounds float, float* %4, i64 %43
  %45 = bitcast float* %44 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %45, align 4, !tbaa !266
  %46 = getelementptr inbounds float, float* %44, i64 4
  %47 = bitcast float* %46 to <4 x float>*
  %wide.load8.3 = load <4 x float>, <4 x float>* %47, align 4, !tbaa !266
  %48 = fadd <4 x float> %broadcast.splat10, %wide.load.3
  %49 = fadd <4 x float> %broadcast.splat12, %wide.load8.3
  %50 = getelementptr inbounds float, float* %5, i64 %43
  %51 = bitcast float* %50 to <4 x float>*
  store <4 x float> %48, <4 x float>* %51, align 4, !tbaa !269
  %52 = getelementptr inbounds float, float* %50, i64 4
  %53 = bitcast float* %52 to <4 x float>*
  store <4 x float> %49, <4 x float>* %53, align 4, !tbaa !269
  %54 = add nuw nsw i64 %10, 32
  %55 = getelementptr inbounds float, float* %4, i64 %54
  %56 = bitcast float* %55 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %56, align 4, !tbaa !266
  %57 = getelementptr inbounds float, float* %55, i64 4
  %58 = bitcast float* %57 to <4 x float>*
  %wide.load8.4 = load <4 x float>, <4 x float>* %58, align 4, !tbaa !266
  %59 = fadd <4 x float> %broadcast.splat10, %wide.load.4
  %60 = fadd <4 x float> %broadcast.splat12, %wide.load8.4
  %61 = getelementptr inbounds float, float* %5, i64 %54
  %62 = bitcast float* %61 to <4 x float>*
  store <4 x float> %59, <4 x float>* %62, align 4, !tbaa !269
  %63 = getelementptr inbounds float, float* %61, i64 4
  %64 = bitcast float* %63 to <4 x float>*
  store <4 x float> %60, <4 x float>* %64, align 4, !tbaa !269
  %65 = add nuw nsw i64 %10, 40
  %66 = getelementptr inbounds float, float* %4, i64 %65
  %67 = bitcast float* %66 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %67, align 4, !tbaa !266
  %68 = getelementptr inbounds float, float* %66, i64 4
  %69 = bitcast float* %68 to <4 x float>*
  %wide.load8.5 = load <4 x float>, <4 x float>* %69, align 4, !tbaa !266
  %70 = fadd <4 x float> %broadcast.splat10, %wide.load.5
  %71 = fadd <4 x float> %broadcast.splat12, %wide.load8.5
  %72 = getelementptr inbounds float, float* %5, i64 %65
  %73 = bitcast float* %72 to <4 x float>*
  store <4 x float> %70, <4 x float>* %73, align 4, !tbaa !269
  %74 = getelementptr inbounds float, float* %72, i64 4
  %75 = bitcast float* %74 to <4 x float>*
  store <4 x float> %71, <4 x float>* %75, align 4, !tbaa !269
  %76 = add nuw nsw i64 %10, 48
  %77 = getelementptr inbounds float, float* %4, i64 %76
  %78 = bitcast float* %77 to <4 x float>*
  %wide.load.6 = load <4 x float>, <4 x float>* %78, align 4, !tbaa !266
  %79 = getelementptr inbounds float, float* %77, i64 4
  %80 = bitcast float* %79 to <4 x float>*
  %wide.load8.6 = load <4 x float>, <4 x float>* %80, align 4, !tbaa !266
  %81 = fadd <4 x float> %broadcast.splat10, %wide.load.6
  %82 = fadd <4 x float> %broadcast.splat12, %wide.load8.6
  %83 = getelementptr inbounds float, float* %5, i64 %76
  %84 = bitcast float* %83 to <4 x float>*
  store <4 x float> %81, <4 x float>* %84, align 4, !tbaa !269
  %85 = getelementptr inbounds float, float* %83, i64 4
  %86 = bitcast float* %85 to <4 x float>*
  store <4 x float> %82, <4 x float>* %86, align 4, !tbaa !269
  %87 = add nuw nsw i64 %10, 56
  %88 = getelementptr inbounds float, float* %4, i64 %87
  %89 = bitcast float* %88 to <4 x float>*
  %wide.load.7 = load <4 x float>, <4 x float>* %89, align 4, !tbaa !266
  %90 = getelementptr inbounds float, float* %88, i64 4
  %91 = bitcast float* %90 to <4 x float>*
  %wide.load8.7 = load <4 x float>, <4 x float>* %91, align 4, !tbaa !266
  %92 = fadd <4 x float> %broadcast.splat10, %wide.load.7
  %93 = fadd <4 x float> %broadcast.splat12, %wide.load8.7
  %94 = getelementptr inbounds float, float* %5, i64 %87
  %95 = bitcast float* %94 to <4 x float>*
  store <4 x float> %92, <4 x float>* %95, align 4, !tbaa !269
  %96 = getelementptr inbounds float, float* %94, i64 4
  %97 = bitcast float* %96 to <4 x float>*
  store <4 x float> %93, <4 x float>* %97, align 4, !tbaa !269
  %98 = add nuw nsw i64 %10, 64
  %99 = getelementptr inbounds float, float* %4, i64 %98
  %100 = bitcast float* %99 to <4 x float>*
  %wide.load.8 = load <4 x float>, <4 x float>* %100, align 4, !tbaa !266
  %101 = getelementptr inbounds float, float* %99, i64 4
  %102 = bitcast float* %101 to <4 x float>*
  %wide.load8.8 = load <4 x float>, <4 x float>* %102, align 4, !tbaa !266
  %103 = fadd <4 x float> %broadcast.splat10, %wide.load.8
  %104 = fadd <4 x float> %broadcast.splat12, %wide.load8.8
  %105 = getelementptr inbounds float, float* %5, i64 %98
  %106 = bitcast float* %105 to <4 x float>*
  store <4 x float> %103, <4 x float>* %106, align 4, !tbaa !269
  %107 = getelementptr inbounds float, float* %105, i64 4
  %108 = bitcast float* %107 to <4 x float>*
  store <4 x float> %104, <4 x float>* %108, align 4, !tbaa !269
  %109 = add nuw nsw i64 %10, 72
  %110 = getelementptr inbounds float, float* %4, i64 %109
  %111 = bitcast float* %110 to <4 x float>*
  %wide.load.9 = load <4 x float>, <4 x float>* %111, align 4, !tbaa !266
  %112 = getelementptr inbounds float, float* %110, i64 4
  %113 = bitcast float* %112 to <4 x float>*
  %wide.load8.9 = load <4 x float>, <4 x float>* %113, align 4, !tbaa !266
  %114 = fadd <4 x float> %broadcast.splat10, %wide.load.9
  %115 = fadd <4 x float> %broadcast.splat12, %wide.load8.9
  %116 = getelementptr inbounds float, float* %5, i64 %109
  %117 = bitcast float* %116 to <4 x float>*
  store <4 x float> %114, <4 x float>* %117, align 4, !tbaa !269
  %118 = getelementptr inbounds float, float* %116, i64 4
  %119 = bitcast float* %118 to <4 x float>*
  store <4 x float> %115, <4 x float>* %119, align 4, !tbaa !269
  %120 = add nuw nsw i64 %10, 80
  %121 = getelementptr inbounds float, float* %4, i64 %120
  %122 = bitcast float* %121 to <4 x float>*
  %wide.load.10 = load <4 x float>, <4 x float>* %122, align 4, !tbaa !266
  %123 = getelementptr inbounds float, float* %121, i64 4
  %124 = bitcast float* %123 to <4 x float>*
  %wide.load8.10 = load <4 x float>, <4 x float>* %124, align 4, !tbaa !266
  %125 = fadd <4 x float> %broadcast.splat10, %wide.load.10
  %126 = fadd <4 x float> %broadcast.splat12, %wide.load8.10
  %127 = getelementptr inbounds float, float* %5, i64 %120
  %128 = bitcast float* %127 to <4 x float>*
  store <4 x float> %125, <4 x float>* %128, align 4, !tbaa !269
  %129 = getelementptr inbounds float, float* %127, i64 4
  %130 = bitcast float* %129 to <4 x float>*
  store <4 x float> %126, <4 x float>* %130, align 4, !tbaa !269
  %131 = add nuw nsw i64 %10, 88
  %132 = getelementptr inbounds float, float* %4, i64 %131
  %133 = bitcast float* %132 to <4 x float>*
  %wide.load.11 = load <4 x float>, <4 x float>* %133, align 4, !tbaa !266
  %134 = getelementptr inbounds float, float* %132, i64 4
  %135 = bitcast float* %134 to <4 x float>*
  %wide.load8.11 = load <4 x float>, <4 x float>* %135, align 4, !tbaa !266
  %136 = fadd <4 x float> %broadcast.splat10, %wide.load.11
  %137 = fadd <4 x float> %broadcast.splat12, %wide.load8.11
  %138 = getelementptr inbounds float, float* %5, i64 %131
  %139 = bitcast float* %138 to <4 x float>*
  store <4 x float> %136, <4 x float>* %139, align 4, !tbaa !269
  %140 = getelementptr inbounds float, float* %138, i64 4
  %141 = bitcast float* %140 to <4 x float>*
  store <4 x float> %137, <4 x float>* %141, align 4, !tbaa !269
  %142 = add nuw nsw i64 %10, 96
  %143 = getelementptr inbounds float, float* %4, i64 %142
  %144 = bitcast float* %143 to <4 x float>*
  %wide.load.12 = load <4 x float>, <4 x float>* %144, align 4, !tbaa !266
  %145 = getelementptr inbounds float, float* %143, i64 4
  %146 = bitcast float* %145 to <4 x float>*
  %wide.load8.12 = load <4 x float>, <4 x float>* %146, align 4, !tbaa !266
  %147 = fadd <4 x float> %broadcast.splat10, %wide.load.12
  %148 = fadd <4 x float> %broadcast.splat12, %wide.load8.12
  %149 = getelementptr inbounds float, float* %5, i64 %142
  %150 = bitcast float* %149 to <4 x float>*
  store <4 x float> %147, <4 x float>* %150, align 4, !tbaa !269
  %151 = getelementptr inbounds float, float* %149, i64 4
  %152 = bitcast float* %151 to <4 x float>*
  store <4 x float> %148, <4 x float>* %152, align 4, !tbaa !269
  %153 = add nuw nsw i64 %10, 104
  %154 = getelementptr inbounds float, float* %4, i64 %153
  %155 = bitcast float* %154 to <4 x float>*
  %wide.load.13 = load <4 x float>, <4 x float>* %155, align 4, !tbaa !266
  %156 = getelementptr inbounds float, float* %154, i64 4
  %157 = bitcast float* %156 to <4 x float>*
  %wide.load8.13 = load <4 x float>, <4 x float>* %157, align 4, !tbaa !266
  %158 = fadd <4 x float> %broadcast.splat10, %wide.load.13
  %159 = fadd <4 x float> %broadcast.splat12, %wide.load8.13
  %160 = getelementptr inbounds float, float* %5, i64 %153
  %161 = bitcast float* %160 to <4 x float>*
  store <4 x float> %158, <4 x float>* %161, align 4, !tbaa !269
  %162 = getelementptr inbounds float, float* %160, i64 4
  %163 = bitcast float* %162 to <4 x float>*
  store <4 x float> %159, <4 x float>* %163, align 4, !tbaa !269
  %164 = add nuw nsw i64 %10, 112
  %165 = getelementptr inbounds float, float* %4, i64 %164
  %166 = bitcast float* %165 to <4 x float>*
  %wide.load.14 = load <4 x float>, <4 x float>* %166, align 4, !tbaa !266
  %167 = getelementptr inbounds float, float* %165, i64 4
  %168 = bitcast float* %167 to <4 x float>*
  %wide.load8.14 = load <4 x float>, <4 x float>* %168, align 4, !tbaa !266
  %169 = fadd <4 x float> %broadcast.splat10, %wide.load.14
  %170 = fadd <4 x float> %broadcast.splat12, %wide.load8.14
  %171 = getelementptr inbounds float, float* %5, i64 %164
  %172 = bitcast float* %171 to <4 x float>*
  store <4 x float> %169, <4 x float>* %172, align 4, !tbaa !269
  %173 = getelementptr inbounds float, float* %171, i64 4
  %174 = bitcast float* %173 to <4 x float>*
  store <4 x float> %170, <4 x float>* %174, align 4, !tbaa !269
  %175 = add nuw nsw i64 %10, 120
  %176 = getelementptr inbounds float, float* %4, i64 %175
  %177 = bitcast float* %176 to <4 x float>*
  %wide.load.15 = load <4 x float>, <4 x float>* %177, align 4, !tbaa !266
  %178 = getelementptr inbounds float, float* %176, i64 4
  %179 = bitcast float* %178 to <4 x float>*
  %wide.load8.15 = load <4 x float>, <4 x float>* %179, align 4, !tbaa !266
  %180 = fadd <4 x float> %broadcast.splat10, %wide.load.15
  %181 = fadd <4 x float> %broadcast.splat12, %wide.load8.15
  %182 = getelementptr inbounds float, float* %5, i64 %175
  %183 = bitcast float* %182 to <4 x float>*
  store <4 x float> %180, <4 x float>* %183, align 4, !tbaa !269
  %184 = getelementptr inbounds float, float* %182, i64 4
  %185 = bitcast float* %184 to <4 x float>*
  store <4 x float> %181, <4 x float>* %185, align 4, !tbaa !269
  %186 = add nuw nsw i64 %10, 128
  %187 = getelementptr inbounds float, float* %4, i64 %186
  %188 = bitcast float* %187 to <4 x float>*
  %wide.load.16 = load <4 x float>, <4 x float>* %188, align 4, !tbaa !266
  %189 = getelementptr inbounds float, float* %187, i64 4
  %190 = bitcast float* %189 to <4 x float>*
  %wide.load8.16 = load <4 x float>, <4 x float>* %190, align 4, !tbaa !266
  %191 = fadd <4 x float> %broadcast.splat10, %wide.load.16
  %192 = fadd <4 x float> %broadcast.splat12, %wide.load8.16
  %193 = getelementptr inbounds float, float* %5, i64 %186
  %194 = bitcast float* %193 to <4 x float>*
  store <4 x float> %191, <4 x float>* %194, align 4, !tbaa !269
  %195 = getelementptr inbounds float, float* %193, i64 4
  %196 = bitcast float* %195 to <4 x float>*
  store <4 x float> %192, <4 x float>* %196, align 4, !tbaa !269
  %197 = add nuw nsw i64 %10, 136
  %198 = getelementptr inbounds float, float* %4, i64 %197
  %199 = bitcast float* %198 to <4 x float>*
  %wide.load.17 = load <4 x float>, <4 x float>* %199, align 4, !tbaa !266
  %200 = getelementptr inbounds float, float* %198, i64 4
  %201 = bitcast float* %200 to <4 x float>*
  %wide.load8.17 = load <4 x float>, <4 x float>* %201, align 4, !tbaa !266
  %202 = fadd <4 x float> %broadcast.splat10, %wide.load.17
  %203 = fadd <4 x float> %broadcast.splat12, %wide.load8.17
  %204 = getelementptr inbounds float, float* %5, i64 %197
  %205 = bitcast float* %204 to <4 x float>*
  store <4 x float> %202, <4 x float>* %205, align 4, !tbaa !269
  %206 = getelementptr inbounds float, float* %204, i64 4
  %207 = bitcast float* %206 to <4 x float>*
  store <4 x float> %203, <4 x float>* %207, align 4, !tbaa !269
  %208 = add nuw nsw i64 %10, 144
  %209 = getelementptr inbounds float, float* %4, i64 %208
  %210 = bitcast float* %209 to <4 x float>*
  %wide.load.18 = load <4 x float>, <4 x float>* %210, align 4, !tbaa !266
  %211 = getelementptr inbounds float, float* %209, i64 4
  %212 = bitcast float* %211 to <4 x float>*
  %wide.load8.18 = load <4 x float>, <4 x float>* %212, align 4, !tbaa !266
  %213 = fadd <4 x float> %broadcast.splat10, %wide.load.18
  %214 = fadd <4 x float> %broadcast.splat12, %wide.load8.18
  %215 = getelementptr inbounds float, float* %5, i64 %208
  %216 = bitcast float* %215 to <4 x float>*
  store <4 x float> %213, <4 x float>* %216, align 4, !tbaa !269
  %217 = getelementptr inbounds float, float* %215, i64 4
  %218 = bitcast float* %217 to <4 x float>*
  store <4 x float> %214, <4 x float>* %218, align 4, !tbaa !269
  %219 = add nuw nsw i64 %10, 152
  %220 = getelementptr inbounds float, float* %4, i64 %219
  %221 = bitcast float* %220 to <4 x float>*
  %wide.load.19 = load <4 x float>, <4 x float>* %221, align 4, !tbaa !266
  %222 = getelementptr inbounds float, float* %220, i64 4
  %223 = bitcast float* %222 to <4 x float>*
  %wide.load8.19 = load <4 x float>, <4 x float>* %223, align 4, !tbaa !266
  %224 = fadd <4 x float> %broadcast.splat10, %wide.load.19
  %225 = fadd <4 x float> %broadcast.splat12, %wide.load8.19
  %226 = getelementptr inbounds float, float* %5, i64 %219
  %227 = bitcast float* %226 to <4 x float>*
  store <4 x float> %224, <4 x float>* %227, align 4, !tbaa !269
  %228 = getelementptr inbounds float, float* %226, i64 4
  %229 = bitcast float* %228 to <4 x float>*
  store <4 x float> %225, <4 x float>* %229, align 4, !tbaa !269
  %230 = add nuw nsw i64 %10, 160
  %231 = getelementptr inbounds float, float* %4, i64 %230
  %232 = bitcast float* %231 to <4 x float>*
  %wide.load.20 = load <4 x float>, <4 x float>* %232, align 4, !tbaa !266
  %233 = getelementptr inbounds float, float* %231, i64 4
  %234 = bitcast float* %233 to <4 x float>*
  %wide.load8.20 = load <4 x float>, <4 x float>* %234, align 4, !tbaa !266
  %235 = fadd <4 x float> %broadcast.splat10, %wide.load.20
  %236 = fadd <4 x float> %broadcast.splat12, %wide.load8.20
  %237 = getelementptr inbounds float, float* %5, i64 %230
  %238 = bitcast float* %237 to <4 x float>*
  store <4 x float> %235, <4 x float>* %238, align 4, !tbaa !269
  %239 = getelementptr inbounds float, float* %237, i64 4
  %240 = bitcast float* %239 to <4 x float>*
  store <4 x float> %236, <4 x float>* %240, align 4, !tbaa !269
  %241 = add nuw nsw i64 %10, 168
  %242 = getelementptr inbounds float, float* %4, i64 %241
  %243 = bitcast float* %242 to <4 x float>*
  %wide.load.21 = load <4 x float>, <4 x float>* %243, align 4, !tbaa !266
  %244 = getelementptr inbounds float, float* %242, i64 4
  %245 = bitcast float* %244 to <4 x float>*
  %wide.load8.21 = load <4 x float>, <4 x float>* %245, align 4, !tbaa !266
  %246 = fadd <4 x float> %broadcast.splat10, %wide.load.21
  %247 = fadd <4 x float> %broadcast.splat12, %wide.load8.21
  %248 = getelementptr inbounds float, float* %5, i64 %241
  %249 = bitcast float* %248 to <4 x float>*
  store <4 x float> %246, <4 x float>* %249, align 4, !tbaa !269
  %250 = getelementptr inbounds float, float* %248, i64 4
  %251 = bitcast float* %250 to <4 x float>*
  store <4 x float> %247, <4 x float>* %251, align 4, !tbaa !269
  %252 = add nuw nsw i64 %10, 176
  %253 = getelementptr inbounds float, float* %4, i64 %252
  %254 = bitcast float* %253 to <4 x float>*
  %wide.load.22 = load <4 x float>, <4 x float>* %254, align 4, !tbaa !266
  %255 = getelementptr inbounds float, float* %253, i64 4
  %256 = bitcast float* %255 to <4 x float>*
  %wide.load8.22 = load <4 x float>, <4 x float>* %256, align 4, !tbaa !266
  %257 = fadd <4 x float> %broadcast.splat10, %wide.load.22
  %258 = fadd <4 x float> %broadcast.splat12, %wide.load8.22
  %259 = getelementptr inbounds float, float* %5, i64 %252
  %260 = bitcast float* %259 to <4 x float>*
  store <4 x float> %257, <4 x float>* %260, align 4, !tbaa !269
  %261 = getelementptr inbounds float, float* %259, i64 4
  %262 = bitcast float* %261 to <4 x float>*
  store <4 x float> %258, <4 x float>* %262, align 4, !tbaa !269
  %263 = add nuw nsw i64 %10, 184
  %264 = getelementptr inbounds float, float* %4, i64 %263
  %265 = bitcast float* %264 to <4 x float>*
  %wide.load.23 = load <4 x float>, <4 x float>* %265, align 4, !tbaa !266
  %266 = getelementptr inbounds float, float* %264, i64 4
  %267 = bitcast float* %266 to <4 x float>*
  %wide.load8.23 = load <4 x float>, <4 x float>* %267, align 4, !tbaa !266
  %268 = fadd <4 x float> %broadcast.splat10, %wide.load.23
  %269 = fadd <4 x float> %broadcast.splat12, %wide.load8.23
  %270 = getelementptr inbounds float, float* %5, i64 %263
  %271 = bitcast float* %270 to <4 x float>*
  store <4 x float> %268, <4 x float>* %271, align 4, !tbaa !269
  %272 = getelementptr inbounds float, float* %270, i64 4
  %273 = bitcast float* %272 to <4 x float>*
  store <4 x float> %269, <4 x float>* %273, align 4, !tbaa !269
  %274 = add nuw nsw i64 %10, 192
  %275 = getelementptr inbounds float, float* %4, i64 %274
  %276 = bitcast float* %275 to <4 x float>*
  %wide.load.24 = load <4 x float>, <4 x float>* %276, align 4, !tbaa !266
  %277 = getelementptr inbounds float, float* %275, i64 4
  %278 = bitcast float* %277 to <4 x float>*
  %wide.load8.24 = load <4 x float>, <4 x float>* %278, align 4, !tbaa !266
  %279 = fadd <4 x float> %broadcast.splat10, %wide.load.24
  %280 = fadd <4 x float> %broadcast.splat12, %wide.load8.24
  %281 = getelementptr inbounds float, float* %5, i64 %274
  %282 = bitcast float* %281 to <4 x float>*
  store <4 x float> %279, <4 x float>* %282, align 4, !tbaa !269
  %283 = getelementptr inbounds float, float* %281, i64 4
  %284 = bitcast float* %283 to <4 x float>*
  store <4 x float> %280, <4 x float>* %284, align 4, !tbaa !269
  %285 = add nuw nsw i64 %10, 200
  %286 = getelementptr inbounds float, float* %4, i64 %285
  %287 = bitcast float* %286 to <4 x float>*
  %wide.load.25 = load <4 x float>, <4 x float>* %287, align 4, !tbaa !266
  %288 = getelementptr inbounds float, float* %286, i64 4
  %289 = bitcast float* %288 to <4 x float>*
  %wide.load8.25 = load <4 x float>, <4 x float>* %289, align 4, !tbaa !266
  %290 = fadd <4 x float> %broadcast.splat10, %wide.load.25
  %291 = fadd <4 x float> %broadcast.splat12, %wide.load8.25
  %292 = getelementptr inbounds float, float* %5, i64 %285
  %293 = bitcast float* %292 to <4 x float>*
  store <4 x float> %290, <4 x float>* %293, align 4, !tbaa !269
  %294 = getelementptr inbounds float, float* %292, i64 4
  %295 = bitcast float* %294 to <4 x float>*
  store <4 x float> %291, <4 x float>* %295, align 4, !tbaa !269
  %296 = add nuw nsw i64 %10, 208
  %297 = getelementptr inbounds float, float* %4, i64 %296
  %298 = bitcast float* %297 to <4 x float>*
  %wide.load.26 = load <4 x float>, <4 x float>* %298, align 4, !tbaa !266
  %299 = getelementptr inbounds float, float* %297, i64 4
  %300 = bitcast float* %299 to <4 x float>*
  %wide.load8.26 = load <4 x float>, <4 x float>* %300, align 4, !tbaa !266
  %301 = fadd <4 x float> %broadcast.splat10, %wide.load.26
  %302 = fadd <4 x float> %broadcast.splat12, %wide.load8.26
  %303 = getelementptr inbounds float, float* %5, i64 %296
  %304 = bitcast float* %303 to <4 x float>*
  store <4 x float> %301, <4 x float>* %304, align 4, !tbaa !269
  %305 = getelementptr inbounds float, float* %303, i64 4
  %306 = bitcast float* %305 to <4 x float>*
  store <4 x float> %302, <4 x float>* %306, align 4, !tbaa !269
  %307 = add nuw nsw i64 %10, 216
  %308 = getelementptr inbounds float, float* %4, i64 %307
  %309 = bitcast float* %308 to <4 x float>*
  %wide.load.27 = load <4 x float>, <4 x float>* %309, align 4, !tbaa !266
  %310 = getelementptr inbounds float, float* %308, i64 4
  %311 = bitcast float* %310 to <4 x float>*
  %wide.load8.27 = load <4 x float>, <4 x float>* %311, align 4, !tbaa !266
  %312 = fadd <4 x float> %broadcast.splat10, %wide.load.27
  %313 = fadd <4 x float> %broadcast.splat12, %wide.load8.27
  %314 = getelementptr inbounds float, float* %5, i64 %307
  %315 = bitcast float* %314 to <4 x float>*
  store <4 x float> %312, <4 x float>* %315, align 4, !tbaa !269
  %316 = getelementptr inbounds float, float* %314, i64 4
  %317 = bitcast float* %316 to <4 x float>*
  store <4 x float> %313, <4 x float>* %317, align 4, !tbaa !269
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 224
  br i1 %exitcond3, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 64
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !34
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

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv1 = phi i64 [ 0, %entry ], [ %indvars.iv.next2, %for_end3 ]
  %6 = mul nuw nsw i64 %indvars.iv1, 784
  %7 = getelementptr inbounds float, float* %3, i64 %indvars.iv1
  %8 = load float, float* %7, align 4, !tbaa !272
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
  %28 = load <4 x float>, <4 x float>* %27, align 4, !tbaa !275
  %29 = fadd <4 x float> %10, %28
  %30 = bitcast float* %26 to <4 x float>*
  store <4 x float> %29, <4 x float>* %30, align 4, !tbaa !278
  %31 = add nuw nsw i64 %24, 4
  %32 = getelementptr inbounds float, float* %4, i64 %31
  %33 = getelementptr inbounds float, float* %5, i64 %31
  %34 = bitcast float* %32 to <4 x float>*
  %35 = load <4 x float>, <4 x float>* %34, align 4, !tbaa !275
  %36 = fadd <4 x float> %12, %35
  %37 = bitcast float* %33 to <4 x float>*
  store <4 x float> %36, <4 x float>* %37, align 4, !tbaa !278
  %38 = add nuw nsw i64 %24, 8
  %39 = getelementptr inbounds float, float* %4, i64 %38
  %40 = getelementptr inbounds float, float* %5, i64 %38
  %41 = bitcast float* %39 to <4 x float>*
  %42 = load <4 x float>, <4 x float>* %41, align 4, !tbaa !275
  %43 = fadd <4 x float> %14, %42
  %44 = bitcast float* %40 to <4 x float>*
  store <4 x float> %43, <4 x float>* %44, align 4, !tbaa !278
  %45 = add nuw nsw i64 %24, 12
  %46 = getelementptr inbounds float, float* %4, i64 %45
  %47 = getelementptr inbounds float, float* %5, i64 %45
  %48 = bitcast float* %46 to <4 x float>*
  %49 = load <4 x float>, <4 x float>* %48, align 4, !tbaa !275
  %50 = fadd <4 x float> %16, %49
  %51 = bitcast float* %47 to <4 x float>*
  store <4 x float> %50, <4 x float>* %51, align 4, !tbaa !278
  %52 = add nuw nsw i64 %24, 16
  %53 = getelementptr inbounds float, float* %4, i64 %52
  %54 = getelementptr inbounds float, float* %5, i64 %52
  %55 = bitcast float* %53 to <4 x float>*
  %56 = load <4 x float>, <4 x float>* %55, align 4, !tbaa !275
  %57 = fadd <4 x float> %18, %56
  %58 = bitcast float* %54 to <4 x float>*
  store <4 x float> %57, <4 x float>* %58, align 4, !tbaa !278
  %59 = add nuw nsw i64 %24, 20
  %60 = getelementptr inbounds float, float* %4, i64 %59
  %61 = getelementptr inbounds float, float* %5, i64 %59
  %62 = bitcast float* %60 to <4 x float>*
  %63 = load <4 x float>, <4 x float>* %62, align 4, !tbaa !275
  %64 = fadd <4 x float> %20, %63
  %65 = bitcast float* %61 to <4 x float>*
  store <4 x float> %64, <4 x float>* %65, align 4, !tbaa !278
  %66 = add nuw nsw i64 %24, 24
  %67 = getelementptr inbounds float, float* %4, i64 %66
  %68 = getelementptr inbounds float, float* %5, i64 %66
  %69 = bitcast float* %67 to <4 x float>*
  %70 = load <4 x float>, <4 x float>* %69, align 4, !tbaa !275
  %71 = fadd <4 x float> %22, %70
  %72 = bitcast float* %68 to <4 x float>*
  store <4 x float> %71, <4 x float>* %72, align 4, !tbaa !278
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 28
  br i1 %exitcond, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 512
  br i1 %exitcond3, label %for_end, label %for_begin1.preheader, !prof !34
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
define private fastcc void @fused_nn_conv2d_3_compute_(i8* noalias nocapture readonly, i8* noalias readonly, i8* noalias) unnamed_addr #3 {
entry:
  %3 = alloca [8 x <32 x float>], align 16
  %4 = alloca [56 x <32 x float>], align 16
  %5 = alloca [18432 x <32 x float>], align 16
  %6 = alloca [861184 x float], align 16
  %7 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar74 = phi i64 [ 0, %entry ], [ %indvar.next75, %for_end3 ]
  %8 = mul nuw nsw i64 %indvar74, 14848
  %9 = trunc i64 %indvar74 to i32
  %10 = add i32 %9, -1
  %11 = icmp ult i32 %10, 56
  %12 = mul nuw nsw i64 %indvar74, 56
  %13 = add nsw i64 %12, -57
  br i1 %11, label %for_begin4.preheader.us, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep81 = getelementptr [861184 x float], [861184 x float]* %6, i64 0, i64 %8
  %scevgep8182 = bitcast float* %scevgep81 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep8182, i8 0, i64 59392, i1 false)
  br label %for_end3

for_begin4.preheader.us:                          ; preds = %for_begin1.preheader, %for_end6.us-lcssa.us.us
  %indvars.iv86 = phi i64 [ %indvars.iv.next87, %for_end6.us-lcssa.us.us ], [ 0, %for_begin1.preheader ]
  %14 = mul nuw nsw i64 %indvars.iv86, 58
  %15 = add nuw nsw i64 %14, %8
  %16 = mul nuw nsw i64 %indvars.iv86, 3136
  %17 = add nsw i64 %13, %16
  br label %for_body5.us.us

for_body5.us.us:                                  ; preds = %if_end.us.us, %for_begin4.preheader.us
  %indvars.iv83 = phi i64 [ 0, %for_begin4.preheader.us ], [ %indvars.iv.next84, %if_end.us.us ]
  %18 = phi i32 [ 0, %for_begin4.preheader.us ], [ %25, %if_end.us.us ]
  %19 = add nuw nsw i64 %15, %indvars.iv83
  %trunc.us.us = trunc i32 %18 to i31
  switch i31 %trunc.us.us, label %if_then.us.us [
    i31 57, label %if_end.us.us
    i31 0, label %if_end.us.us
  ]

if_then.us.us:                                    ; preds = %for_body5.us.us
  %20 = add nsw i64 %17, %indvars.iv83
  %21 = getelementptr inbounds float, float* %7, i64 %20
  %22 = load float, float* %21, align 4, !tbaa !281
  br label %if_end.us.us

if_end.us.us:                                     ; preds = %if_then.us.us, %for_body5.us.us, %for_body5.us.us
  %23 = phi float [ %22, %if_then.us.us ], [ 0.000000e+00, %for_body5.us.us ], [ 0.000000e+00, %for_body5.us.us ]
  %24 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %19
  store float %23, float* %24, align 4, !tbaa !284
  %indvars.iv.next84 = add nuw nsw i64 %indvars.iv83, 1
  %25 = add nuw nsw i32 %18, 1
  %exitcond85 = icmp eq i64 %indvars.iv.next84, 58
  br i1 %exitcond85, label %for_end6.us-lcssa.us.us, label %for_body5.us.us, !prof !34

for_end6.us-lcssa.us.us:                          ; preds = %if_end.us.us
  %indvars.iv.next87 = add nuw nsw i64 %indvars.iv86, 1
  %exitcond88 = icmp eq i64 %indvars.iv.next87, 256
  br i1 %exitcond88, label %for_end3, label %for_begin4.preheader.us, !prof !34

for_begin7.preheader:                             ; preds = %for_end3
  %26 = bitcast [8 x <32 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0
  %27 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_end3:                                         ; preds = %for_end6.us-lcssa.us.us, %for_begin4.preheader.preheader
  %indvar.next75 = add nuw nsw i64 %indvar74, 1
  %exitcond90 = icmp eq i64 %indvar.next75, 58
  br i1 %exitcond90, label %for_begin7.preheader, label %for_begin1.preheader, !prof !34

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %indvars.iv68 = phi i64 [ 0, %for_begin7.preheader ], [ %indvars.iv.next69, %for_end12 ]
  %28 = mul nuw nsw i64 %indvars.iv68, 24576
  %29 = trunc i64 %indvars.iv68 to i32
  %30 = urem i32 %29, 3
  %31 = mul nuw nsw i32 %30, 3
  %32 = udiv i32 %29, 3
  %33 = mul nsw i32 %32, 73728
  %34 = or i32 %31, %33
  %35 = zext i32 %34 to i64
  br label %for_begin13.preheader

for_begin16.preheader:                            ; preds = %for_end12
  %36 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 32
  %37 = bitcast float* %36 to <32 x float>*
  %38 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 64
  %39 = bitcast float* %38 to <32 x float>*
  %40 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 96
  %41 = bitcast float* %40 to <32 x float>*
  %42 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 128
  %43 = bitcast float* %42 to <32 x float>*
  %44 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 160
  %45 = bitcast float* %44 to <32 x float>*
  %46 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 192
  %47 = bitcast float* %46 to <32 x float>*
  %48 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 224
  %49 = bitcast float* %48 to <32 x float>*
  %50 = bitcast i8* %2 to float*
  %51 = bitcast [8 x <32 x float>]* %3 to i8*
  br label %for_begin19.preheader

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvars.iv65 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next66, %for_end15 ]
  %52 = shl i64 %indvars.iv65, 13
  %53 = add nuw nsw i64 %52, %28
  %54 = add nuw nsw i64 %indvars.iv65, %35
  br label %for_body14

for_end12:                                        ; preds = %for_end15
  %indvars.iv.next69 = add nuw nsw i64 %indvars.iv68, 1
  %exitcond70 = icmp eq i64 %indvars.iv.next69, 24
  br i1 %exitcond70, label %for_begin16.preheader, label %for_begin10.preheader, !prof !34

for_body14:                                       ; preds = %for_body14, %for_begin13.preheader
  %indvars.iv62 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next63, %for_body14 ]
  %55 = shl i64 %indvars.iv62, 5
  %56 = add nuw nsw i64 %53, %55
  %57 = mul nuw nsw i64 %indvars.iv62, 9
  %58 = add nuw nsw i64 %54, %57
  %59 = add nuw nsw i64 %58, 2304
  %60 = add nuw nsw i64 %58, 4608
  %61 = add nuw nsw i64 %58, 6912
  %62 = add nuw nsw i64 %58, 9216
  %63 = add nuw nsw i64 %58, 11520
  %64 = add nuw nsw i64 %58, 13824
  %65 = add nuw nsw i64 %58, 16128
  %66 = add nuw nsw i64 %58, 18432
  %67 = add nuw nsw i64 %58, 20736
  %68 = add nuw nsw i64 %58, 23040
  %69 = add nuw nsw i64 %58, 25344
  %70 = add nuw nsw i64 %58, 27648
  %71 = add nuw nsw i64 %58, 29952
  %72 = add nuw nsw i64 %58, 32256
  %73 = add nuw nsw i64 %58, 34560
  %74 = add nuw nsw i64 %58, 36864
  %75 = add nuw nsw i64 %58, 39168
  %76 = add nuw nsw i64 %58, 41472
  %77 = add nuw nsw i64 %58, 43776
  %78 = add nuw nsw i64 %58, 46080
  %79 = add nuw nsw i64 %58, 48384
  %80 = add nuw nsw i64 %58, 50688
  %81 = add nuw nsw i64 %58, 52992
  %82 = add nuw nsw i64 %58, 55296
  %83 = add nuw nsw i64 %58, 57600
  %84 = add nuw nsw i64 %58, 59904
  %85 = add nuw nsw i64 %58, 62208
  %86 = add nuw nsw i64 %58, 64512
  %87 = add nuw nsw i64 %58, 66816
  %88 = add nuw nsw i64 %58, 69120
  %89 = add nuw nsw i64 %58, 71424
  %90 = getelementptr inbounds float, float* %27, i64 %58
  %91 = load float, float* %90, align 4, !tbaa !287
  %92 = insertelement <32 x float> undef, float %91, i32 0
  %93 = getelementptr inbounds float, float* %27, i64 %59
  %94 = load float, float* %93, align 4, !tbaa !287
  %95 = insertelement <32 x float> %92, float %94, i32 1
  %96 = getelementptr inbounds float, float* %27, i64 %60
  %97 = load float, float* %96, align 4, !tbaa !287
  %98 = insertelement <32 x float> %95, float %97, i32 2
  %99 = getelementptr inbounds float, float* %27, i64 %61
  %100 = load float, float* %99, align 4, !tbaa !287
  %101 = insertelement <32 x float> %98, float %100, i32 3
  %102 = getelementptr inbounds float, float* %27, i64 %62
  %103 = load float, float* %102, align 4, !tbaa !287
  %104 = insertelement <32 x float> %101, float %103, i32 4
  %105 = getelementptr inbounds float, float* %27, i64 %63
  %106 = load float, float* %105, align 4, !tbaa !287
  %107 = insertelement <32 x float> %104, float %106, i32 5
  %108 = getelementptr inbounds float, float* %27, i64 %64
  %109 = load float, float* %108, align 4, !tbaa !287
  %110 = insertelement <32 x float> %107, float %109, i32 6
  %111 = getelementptr inbounds float, float* %27, i64 %65
  %112 = load float, float* %111, align 4, !tbaa !287
  %113 = insertelement <32 x float> %110, float %112, i32 7
  %114 = getelementptr inbounds float, float* %27, i64 %66
  %115 = load float, float* %114, align 4, !tbaa !287
  %116 = insertelement <32 x float> %113, float %115, i32 8
  %117 = getelementptr inbounds float, float* %27, i64 %67
  %118 = load float, float* %117, align 4, !tbaa !287
  %119 = insertelement <32 x float> %116, float %118, i32 9
  %120 = getelementptr inbounds float, float* %27, i64 %68
  %121 = load float, float* %120, align 4, !tbaa !287
  %122 = insertelement <32 x float> %119, float %121, i32 10
  %123 = getelementptr inbounds float, float* %27, i64 %69
  %124 = load float, float* %123, align 4, !tbaa !287
  %125 = insertelement <32 x float> %122, float %124, i32 11
  %126 = getelementptr inbounds float, float* %27, i64 %70
  %127 = load float, float* %126, align 4, !tbaa !287
  %128 = insertelement <32 x float> %125, float %127, i32 12
  %129 = getelementptr inbounds float, float* %27, i64 %71
  %130 = load float, float* %129, align 4, !tbaa !287
  %131 = insertelement <32 x float> %128, float %130, i32 13
  %132 = getelementptr inbounds float, float* %27, i64 %72
  %133 = load float, float* %132, align 4, !tbaa !287
  %134 = insertelement <32 x float> %131, float %133, i32 14
  %135 = getelementptr inbounds float, float* %27, i64 %73
  %136 = load float, float* %135, align 4, !tbaa !287
  %137 = insertelement <32 x float> %134, float %136, i32 15
  %138 = getelementptr inbounds float, float* %27, i64 %74
  %139 = load float, float* %138, align 4, !tbaa !287
  %140 = insertelement <32 x float> %137, float %139, i32 16
  %141 = getelementptr inbounds float, float* %27, i64 %75
  %142 = load float, float* %141, align 4, !tbaa !287
  %143 = insertelement <32 x float> %140, float %142, i32 17
  %144 = getelementptr inbounds float, float* %27, i64 %76
  %145 = load float, float* %144, align 4, !tbaa !287
  %146 = insertelement <32 x float> %143, float %145, i32 18
  %147 = getelementptr inbounds float, float* %27, i64 %77
  %148 = load float, float* %147, align 4, !tbaa !287
  %149 = insertelement <32 x float> %146, float %148, i32 19
  %150 = getelementptr inbounds float, float* %27, i64 %78
  %151 = load float, float* %150, align 4, !tbaa !287
  %152 = insertelement <32 x float> %149, float %151, i32 20
  %153 = getelementptr inbounds float, float* %27, i64 %79
  %154 = load float, float* %153, align 4, !tbaa !287
  %155 = insertelement <32 x float> %152, float %154, i32 21
  %156 = getelementptr inbounds float, float* %27, i64 %80
  %157 = load float, float* %156, align 4, !tbaa !287
  %158 = insertelement <32 x float> %155, float %157, i32 22
  %159 = getelementptr inbounds float, float* %27, i64 %81
  %160 = load float, float* %159, align 4, !tbaa !287
  %161 = insertelement <32 x float> %158, float %160, i32 23
  %162 = getelementptr inbounds float, float* %27, i64 %82
  %163 = load float, float* %162, align 4, !tbaa !287
  %164 = insertelement <32 x float> %161, float %163, i32 24
  %165 = getelementptr inbounds float, float* %27, i64 %83
  %166 = load float, float* %165, align 4, !tbaa !287
  %167 = insertelement <32 x float> %164, float %166, i32 25
  %168 = getelementptr inbounds float, float* %27, i64 %84
  %169 = load float, float* %168, align 4, !tbaa !287
  %170 = insertelement <32 x float> %167, float %169, i32 26
  %171 = getelementptr inbounds float, float* %27, i64 %85
  %172 = load float, float* %171, align 4, !tbaa !287
  %173 = insertelement <32 x float> %170, float %172, i32 27
  %174 = getelementptr inbounds float, float* %27, i64 %86
  %175 = load float, float* %174, align 4, !tbaa !287
  %176 = insertelement <32 x float> %173, float %175, i32 28
  %177 = getelementptr inbounds float, float* %27, i64 %87
  %178 = load float, float* %177, align 4, !tbaa !287
  %179 = insertelement <32 x float> %176, float %178, i32 29
  %180 = getelementptr inbounds float, float* %27, i64 %88
  %181 = load float, float* %180, align 4, !tbaa !287
  %182 = insertelement <32 x float> %179, float %181, i32 30
  %183 = getelementptr inbounds float, float* %27, i64 %89
  %184 = load float, float* %183, align 4, !tbaa !287
  %185 = insertelement <32 x float> %182, float %184, i32 31
  %186 = getelementptr inbounds [18432 x <32 x float>], [18432 x <32 x float>]* %5, i64 0, i64 0, i64 %56
  %187 = bitcast float* %186 to <32 x float>*
  store <32 x float> %185, <32 x float>* %187, align 16, !tbaa !290
  %indvars.iv.next63 = add nuw nsw i64 %indvars.iv62, 1
  %exitcond64 = icmp eq i64 %indvars.iv.next63, 256
  br i1 %exitcond64, label %for_end15, label %for_body14, !prof !34

for_end15:                                        ; preds = %for_body14
  %indvars.iv.next66 = add nuw nsw i64 %indvars.iv65, 1
  %exitcond67 = icmp eq i64 %indvars.iv.next66, 3
  br i1 %exitcond67, label %for_end12, label %for_begin13.preheader, !prof !34

for_begin19.preheader:                            ; preds = %for_end33, %for_begin16.preheader
  %188 = phi i32 [ 0, %for_begin16.preheader ], [ %316, %for_end33 ]
  %189 = urem i32 %188, 56
  %190 = udiv i32 %188, 56
  %191 = mul nsw i32 %190, 73728
  %192 = zext i32 %191 to i64
  %193 = mul nuw nsw i32 %189, 14848
  %194 = zext i32 %193 to i64
  %195 = mul nuw nsw i32 %189, 14848
  %196 = add nuw nsw i32 %195, 14848
  %197 = zext i32 %196 to i64
  %198 = add nuw nsw i64 %192, 24576
  %199 = mul nuw nsw i32 %189, 14848
  %200 = add nuw nsw i32 %199, 29696
  %201 = zext i32 %200 to i64
  %202 = add nuw nsw i64 %192, 49152
  br label %for_body20

for_end18:                                        ; preds = %for_end33
  ret void

for_begin31.preheader:                            ; preds = %for_end27.2
  %203 = mul nuw nsw i32 %189, 56
  %204 = mul nsw i32 %190, 100352
  %205 = add nuw nsw i32 %204, %203
  %206 = zext i32 %205 to i64
  br label %for_begin34.preheader

for_body20:                                       ; preds = %for_end27.2, %for_begin19.preheader
  %indvar = phi i64 [ 0, %for_begin19.preheader ], [ %indvar.next, %for_end27.2 ]
  %207 = shl i64 %indvar, 3
  %scevgep = getelementptr [56 x <32 x float>], [56 x <32 x float>]* %4, i64 0, i64 %207
  %scevgep53 = bitcast <32 x float>* %scevgep to i8*
  %208 = add nuw nsw i64 %207, %194
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %51, i8 0, i64 1024, i1 false)
  %209 = trunc i64 %208 to i32
  br label %for_body26

for_body26:                                       ; preds = %for_body26, %for_body20
  %indvars.iv = phi i64 [ 0, %for_body20 ], [ %indvars.iv.next, %for_body26 ]
  %210 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %309, %for_body26 ]
  %211 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %303, %for_body26 ]
  %212 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %302, %for_body26 ]
  %213 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %301, %for_body26 ]
  %214 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %300, %for_body26 ]
  %215 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %299, %for_body26 ]
  %216 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %298, %for_body26 ]
  %217 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %297, %for_body26 ]
  %218 = phi i32 [ 0, %for_body20 ], [ %310, %for_body26 ]
  %219 = mul nuw nsw i64 %indvars.iv, 58
  %220 = mul nuw nsw i32 %218, 58
  %221 = add nsw i64 %208, %219
  %222 = add nsw i32 %220, %209
  %223 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %221
  %224 = load float, float* %223, align 8, !tbaa !284
  %225 = insertelement <32 x float> undef, float %224, i32 0
  %226 = shufflevector <32 x float> %225, <32 x float> undef, <32 x i32> zeroinitializer
  %227 = shl nsw i64 %indvars.iv, 5
  %228 = add nuw nsw i64 %227, %192
  %229 = getelementptr inbounds [18432 x <32 x float>], [18432 x <32 x float>]* %5, i64 0, i64 0, i64 %228
  %230 = bitcast float* %229 to <32 x float>*
  %231 = load <32 x float>, <32 x float>* %230, align 16, !tbaa !290
  %232 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %226, <32 x float> %231, <32 x float> %217)
  %233 = or i32 %222, 1
  %234 = sext i32 %233 to i64
  %235 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %234
  %236 = load float, float* %235, align 4, !tbaa !284
  %237 = insertelement <32 x float> undef, float %236, i32 0
  %238 = shufflevector <32 x float> %237, <32 x float> undef, <32 x i32> zeroinitializer
  %239 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %238, <32 x float> %231, <32 x float> %216)
  %240 = add nsw i64 %221, 2
  %241 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %240
  %242 = load float, float* %241, align 8, !tbaa !284
  %243 = insertelement <32 x float> undef, float %242, i32 0
  %244 = shufflevector <32 x float> %243, <32 x float> undef, <32 x i32> zeroinitializer
  %245 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %244, <32 x float> %231, <32 x float> %215)
  %246 = add nsw i64 %221, 3
  %247 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %246
  %248 = load float, float* %247, align 4, !tbaa !284
  %249 = insertelement <32 x float> undef, float %248, i32 0
  %250 = shufflevector <32 x float> %249, <32 x float> undef, <32 x i32> zeroinitializer
  %251 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %250, <32 x float> %231, <32 x float> %214)
  %252 = add nsw i64 %221, 4
  %253 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %252
  %254 = load float, float* %253, align 8, !tbaa !284
  %255 = insertelement <32 x float> undef, float %254, i32 0
  %256 = shufflevector <32 x float> %255, <32 x float> undef, <32 x i32> zeroinitializer
  %257 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %256, <32 x float> %231, <32 x float> %213)
  %258 = add nsw i64 %221, 5
  %259 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %258
  %260 = load float, float* %259, align 4, !tbaa !284
  %261 = insertelement <32 x float> undef, float %260, i32 0
  %262 = shufflevector <32 x float> %261, <32 x float> undef, <32 x i32> zeroinitializer
  %263 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %262, <32 x float> %231, <32 x float> %212)
  %264 = add nsw i64 %221, 6
  %265 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %264
  %266 = load float, float* %265, align 8, !tbaa !284
  %267 = insertelement <32 x float> undef, float %266, i32 0
  %268 = shufflevector <32 x float> %267, <32 x float> undef, <32 x i32> zeroinitializer
  %269 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %268, <32 x float> %231, <32 x float> %211)
  %270 = add nsw i64 %221, 7
  %271 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %270
  %272 = load float, float* %271, align 4, !tbaa !284
  %273 = insertelement <32 x float> undef, float %272, i32 0
  %274 = shufflevector <32 x float> %273, <32 x float> undef, <32 x i32> zeroinitializer
  %275 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %274, <32 x float> %231, <32 x float> %210)
  %276 = add nuw nsw i64 %228, 8192
  %277 = getelementptr inbounds [18432 x <32 x float>], [18432 x <32 x float>]* %5, i64 0, i64 0, i64 %276
  %278 = bitcast float* %277 to <32 x float>*
  %279 = load <32 x float>, <32 x float>* %278, align 16, !tbaa !290
  %280 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %238, <32 x float> %279, <32 x float> %232)
  %281 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %244, <32 x float> %279, <32 x float> %239)
  %282 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %250, <32 x float> %279, <32 x float> %245)
  %283 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %256, <32 x float> %279, <32 x float> %251)
  %284 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %262, <32 x float> %279, <32 x float> %257)
  %285 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %268, <32 x float> %279, <32 x float> %263)
  %286 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %274, <32 x float> %279, <32 x float> %269)
  %287 = add nsw i64 %221, 8
  %288 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %287
  %289 = load float, float* %288, align 8, !tbaa !284
  %290 = insertelement <32 x float> undef, float %289, i32 0
  %291 = shufflevector <32 x float> %290, <32 x float> undef, <32 x i32> zeroinitializer
  %292 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %291, <32 x float> %279, <32 x float> %275)
  %293 = add nuw nsw i64 %228, 16384
  %294 = getelementptr inbounds [18432 x <32 x float>], [18432 x <32 x float>]* %5, i64 0, i64 0, i64 %293
  %295 = bitcast float* %294 to <32 x float>*
  %296 = load <32 x float>, <32 x float>* %295, align 16, !tbaa !290
  %297 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %244, <32 x float> %296, <32 x float> %280)
  %298 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %250, <32 x float> %296, <32 x float> %281)
  %299 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %256, <32 x float> %296, <32 x float> %282)
  %300 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %262, <32 x float> %296, <32 x float> %283)
  %301 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %268, <32 x float> %296, <32 x float> %284)
  %302 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %274, <32 x float> %296, <32 x float> %285)
  %303 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %291, <32 x float> %296, <32 x float> %286)
  %304 = add nsw i64 %221, 9
  %305 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %304
  %306 = load float, float* %305, align 4, !tbaa !284
  %307 = insertelement <32 x float> undef, float %306, i32 0
  %308 = shufflevector <32 x float> %307, <32 x float> undef, <32 x i32> zeroinitializer
  %309 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %308, <32 x float> %296, <32 x float> %292)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %310 = add nuw nsw i32 %218, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end27, label %for_body26, !prof !34

for_end27:                                        ; preds = %for_body26
  %311 = add nuw nsw i64 %207, %197
  %312 = trunc i64 %311 to i32
  br label %for_body26.1

for_begin34.preheader:                            ; preds = %for_end36, %for_begin31.preheader
  %indvars.iv58 = phi i64 [ 0, %for_begin31.preheader ], [ %indvars.iv.next59, %for_end36 ]
  %313 = shl i64 %indvars.iv58, 3
  %314 = add nuw nsw i64 %313, %206
  %315 = shl i64 %indvars.iv58, 8
  br label %for_body35

for_end33:                                        ; preds = %for_end36
  %316 = add nuw nsw i32 %188, 1
  %exitcond61 = icmp eq i32 %316, 448
  br i1 %exitcond61, label %for_end18, label %for_begin19.preheader, !prof !34

for_body35:                                       ; preds = %for_body35, %for_begin34.preheader
  %indvars.iv55 = phi i64 [ 0, %for_begin34.preheader ], [ %indvars.iv.next56, %for_body35 ]
  %317 = add nuw nsw i64 %314, %indvars.iv55
  %318 = add nuw nsw i64 %317, 3136
  %319 = add nuw nsw i64 %317, 6272
  %320 = add nuw nsw i64 %317, 9408
  %321 = add nuw nsw i64 %317, 12544
  %322 = add nuw nsw i64 %317, 15680
  %323 = add nuw nsw i64 %317, 18816
  %324 = add nuw nsw i64 %317, 21952
  %325 = add nuw nsw i64 %317, 25088
  %326 = add nuw nsw i64 %317, 28224
  %327 = add nuw nsw i64 %317, 31360
  %328 = add nuw nsw i64 %317, 34496
  %329 = add nuw nsw i64 %317, 37632
  %330 = add nuw nsw i64 %317, 40768
  %331 = add nuw nsw i64 %317, 43904
  %332 = add nuw nsw i64 %317, 47040
  %333 = add nuw nsw i64 %317, 50176
  %334 = add nuw nsw i64 %317, 53312
  %335 = add nuw nsw i64 %317, 56448
  %336 = add nuw nsw i64 %317, 59584
  %337 = add nuw nsw i64 %317, 62720
  %338 = add nuw nsw i64 %317, 65856
  %339 = add nuw nsw i64 %317, 68992
  %340 = add nuw nsw i64 %317, 72128
  %341 = add nuw nsw i64 %317, 75264
  %342 = add nuw nsw i64 %317, 78400
  %343 = add nuw nsw i64 %317, 81536
  %344 = add nuw nsw i64 %317, 84672
  %345 = add nuw nsw i64 %317, 87808
  %346 = add nuw nsw i64 %317, 90944
  %347 = add nuw nsw i64 %317, 94080
  %348 = add nuw nsw i64 %317, 97216
  %349 = shl i64 %indvars.iv55, 5
  %350 = add nuw nsw i64 %349, %315
  %351 = getelementptr inbounds [56 x <32 x float>], [56 x <32 x float>]* %4, i64 0, i64 0, i64 %350
  %352 = bitcast float* %351 to <32 x float>*
  %353 = load <32 x float>, <32 x float>* %352, align 16, !tbaa !293
  %354 = getelementptr inbounds float, float* %50, i64 %317
  %355 = extractelement <32 x float> %353, i64 0
  store float %355, float* %354, align 4, !tbaa !296
  %356 = getelementptr inbounds float, float* %50, i64 %318
  %357 = extractelement <32 x float> %353, i64 1
  store float %357, float* %356, align 4, !tbaa !296
  %358 = getelementptr inbounds float, float* %50, i64 %319
  %359 = extractelement <32 x float> %353, i64 2
  store float %359, float* %358, align 4, !tbaa !296
  %360 = getelementptr inbounds float, float* %50, i64 %320
  %361 = extractelement <32 x float> %353, i64 3
  store float %361, float* %360, align 4, !tbaa !296
  %362 = getelementptr inbounds float, float* %50, i64 %321
  %363 = extractelement <32 x float> %353, i64 4
  store float %363, float* %362, align 4, !tbaa !296
  %364 = getelementptr inbounds float, float* %50, i64 %322
  %365 = extractelement <32 x float> %353, i64 5
  store float %365, float* %364, align 4, !tbaa !296
  %366 = getelementptr inbounds float, float* %50, i64 %323
  %367 = extractelement <32 x float> %353, i64 6
  store float %367, float* %366, align 4, !tbaa !296
  %368 = getelementptr inbounds float, float* %50, i64 %324
  %369 = extractelement <32 x float> %353, i64 7
  store float %369, float* %368, align 4, !tbaa !296
  %370 = getelementptr inbounds float, float* %50, i64 %325
  %371 = extractelement <32 x float> %353, i64 8
  store float %371, float* %370, align 4, !tbaa !296
  %372 = getelementptr inbounds float, float* %50, i64 %326
  %373 = extractelement <32 x float> %353, i64 9
  store float %373, float* %372, align 4, !tbaa !296
  %374 = getelementptr inbounds float, float* %50, i64 %327
  %375 = extractelement <32 x float> %353, i64 10
  store float %375, float* %374, align 4, !tbaa !296
  %376 = getelementptr inbounds float, float* %50, i64 %328
  %377 = extractelement <32 x float> %353, i64 11
  store float %377, float* %376, align 4, !tbaa !296
  %378 = getelementptr inbounds float, float* %50, i64 %329
  %379 = extractelement <32 x float> %353, i64 12
  store float %379, float* %378, align 4, !tbaa !296
  %380 = getelementptr inbounds float, float* %50, i64 %330
  %381 = extractelement <32 x float> %353, i64 13
  store float %381, float* %380, align 4, !tbaa !296
  %382 = getelementptr inbounds float, float* %50, i64 %331
  %383 = extractelement <32 x float> %353, i64 14
  store float %383, float* %382, align 4, !tbaa !296
  %384 = getelementptr inbounds float, float* %50, i64 %332
  %385 = extractelement <32 x float> %353, i64 15
  store float %385, float* %384, align 4, !tbaa !296
  %386 = getelementptr inbounds float, float* %50, i64 %333
  %387 = extractelement <32 x float> %353, i64 16
  store float %387, float* %386, align 4, !tbaa !296
  %388 = getelementptr inbounds float, float* %50, i64 %334
  %389 = extractelement <32 x float> %353, i64 17
  store float %389, float* %388, align 4, !tbaa !296
  %390 = getelementptr inbounds float, float* %50, i64 %335
  %391 = extractelement <32 x float> %353, i64 18
  store float %391, float* %390, align 4, !tbaa !296
  %392 = getelementptr inbounds float, float* %50, i64 %336
  %393 = extractelement <32 x float> %353, i64 19
  store float %393, float* %392, align 4, !tbaa !296
  %394 = getelementptr inbounds float, float* %50, i64 %337
  %395 = extractelement <32 x float> %353, i64 20
  store float %395, float* %394, align 4, !tbaa !296
  %396 = getelementptr inbounds float, float* %50, i64 %338
  %397 = extractelement <32 x float> %353, i64 21
  store float %397, float* %396, align 4, !tbaa !296
  %398 = getelementptr inbounds float, float* %50, i64 %339
  %399 = extractelement <32 x float> %353, i64 22
  store float %399, float* %398, align 4, !tbaa !296
  %400 = getelementptr inbounds float, float* %50, i64 %340
  %401 = extractelement <32 x float> %353, i64 23
  store float %401, float* %400, align 4, !tbaa !296
  %402 = getelementptr inbounds float, float* %50, i64 %341
  %403 = extractelement <32 x float> %353, i64 24
  store float %403, float* %402, align 4, !tbaa !296
  %404 = getelementptr inbounds float, float* %50, i64 %342
  %405 = extractelement <32 x float> %353, i64 25
  store float %405, float* %404, align 4, !tbaa !296
  %406 = getelementptr inbounds float, float* %50, i64 %343
  %407 = extractelement <32 x float> %353, i64 26
  store float %407, float* %406, align 4, !tbaa !296
  %408 = getelementptr inbounds float, float* %50, i64 %344
  %409 = extractelement <32 x float> %353, i64 27
  store float %409, float* %408, align 4, !tbaa !296
  %410 = getelementptr inbounds float, float* %50, i64 %345
  %411 = extractelement <32 x float> %353, i64 28
  store float %411, float* %410, align 4, !tbaa !296
  %412 = getelementptr inbounds float, float* %50, i64 %346
  %413 = extractelement <32 x float> %353, i64 29
  store float %413, float* %412, align 4, !tbaa !296
  %414 = getelementptr inbounds float, float* %50, i64 %347
  %415 = extractelement <32 x float> %353, i64 30
  store float %415, float* %414, align 4, !tbaa !296
  %416 = getelementptr inbounds float, float* %50, i64 %348
  %417 = extractelement <32 x float> %353, i64 31
  store float %417, float* %416, align 4, !tbaa !296
  %indvars.iv.next56 = add nuw nsw i64 %indvars.iv55, 1
  %exitcond57 = icmp eq i64 %indvars.iv.next56, 8
  br i1 %exitcond57, label %for_end36, label %for_body35, !prof !34

for_end36:                                        ; preds = %for_body35
  %indvars.iv.next59 = add nuw nsw i64 %indvars.iv58, 1
  %exitcond60 = icmp eq i64 %indvars.iv.next59, 7
  br i1 %exitcond60, label %for_end33, label %for_begin34.preheader, !prof !34

for_body26.1:                                     ; preds = %for_body26.1, %for_end27
  %indvars.iv.1 = phi i64 [ 0, %for_end27 ], [ %indvars.iv.next.1, %for_body26.1 ]
  %418 = phi <32 x float> [ %309, %for_end27 ], [ %517, %for_body26.1 ]
  %419 = phi <32 x float> [ %303, %for_end27 ], [ %511, %for_body26.1 ]
  %420 = phi <32 x float> [ %302, %for_end27 ], [ %510, %for_body26.1 ]
  %421 = phi <32 x float> [ %301, %for_end27 ], [ %509, %for_body26.1 ]
  %422 = phi <32 x float> [ %300, %for_end27 ], [ %508, %for_body26.1 ]
  %423 = phi <32 x float> [ %299, %for_end27 ], [ %507, %for_body26.1 ]
  %424 = phi <32 x float> [ %298, %for_end27 ], [ %506, %for_body26.1 ]
  %425 = phi <32 x float> [ %297, %for_end27 ], [ %505, %for_body26.1 ]
  %426 = phi i32 [ 0, %for_end27 ], [ %518, %for_body26.1 ]
  %427 = mul nuw nsw i64 %indvars.iv.1, 58
  %428 = mul nuw nsw i32 %426, 58
  %429 = add nsw i64 %311, %427
  %430 = add nsw i32 %428, %312
  %431 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %429
  %432 = load float, float* %431, align 8, !tbaa !284
  %433 = insertelement <32 x float> undef, float %432, i32 0
  %434 = shufflevector <32 x float> %433, <32 x float> undef, <32 x i32> zeroinitializer
  %435 = shl nsw i64 %indvars.iv.1, 5
  %436 = add nuw nsw i64 %198, %435
  %437 = getelementptr inbounds [18432 x <32 x float>], [18432 x <32 x float>]* %5, i64 0, i64 0, i64 %436
  %438 = bitcast float* %437 to <32 x float>*
  %439 = load <32 x float>, <32 x float>* %438, align 16, !tbaa !290
  %440 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %434, <32 x float> %439, <32 x float> %425)
  %441 = or i32 %430, 1
  %442 = sext i32 %441 to i64
  %443 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %442
  %444 = load float, float* %443, align 4, !tbaa !284
  %445 = insertelement <32 x float> undef, float %444, i32 0
  %446 = shufflevector <32 x float> %445, <32 x float> undef, <32 x i32> zeroinitializer
  %447 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %446, <32 x float> %439, <32 x float> %424)
  %448 = add nsw i64 %429, 2
  %449 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %448
  %450 = load float, float* %449, align 8, !tbaa !284
  %451 = insertelement <32 x float> undef, float %450, i32 0
  %452 = shufflevector <32 x float> %451, <32 x float> undef, <32 x i32> zeroinitializer
  %453 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %452, <32 x float> %439, <32 x float> %423)
  %454 = add nsw i64 %429, 3
  %455 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %454
  %456 = load float, float* %455, align 4, !tbaa !284
  %457 = insertelement <32 x float> undef, float %456, i32 0
  %458 = shufflevector <32 x float> %457, <32 x float> undef, <32 x i32> zeroinitializer
  %459 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %458, <32 x float> %439, <32 x float> %422)
  %460 = add nsw i64 %429, 4
  %461 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %460
  %462 = load float, float* %461, align 8, !tbaa !284
  %463 = insertelement <32 x float> undef, float %462, i32 0
  %464 = shufflevector <32 x float> %463, <32 x float> undef, <32 x i32> zeroinitializer
  %465 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %464, <32 x float> %439, <32 x float> %421)
  %466 = add nsw i64 %429, 5
  %467 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %466
  %468 = load float, float* %467, align 4, !tbaa !284
  %469 = insertelement <32 x float> undef, float %468, i32 0
  %470 = shufflevector <32 x float> %469, <32 x float> undef, <32 x i32> zeroinitializer
  %471 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %470, <32 x float> %439, <32 x float> %420)
  %472 = add nsw i64 %429, 6
  %473 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %472
  %474 = load float, float* %473, align 8, !tbaa !284
  %475 = insertelement <32 x float> undef, float %474, i32 0
  %476 = shufflevector <32 x float> %475, <32 x float> undef, <32 x i32> zeroinitializer
  %477 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %476, <32 x float> %439, <32 x float> %419)
  %478 = add nsw i64 %429, 7
  %479 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %478
  %480 = load float, float* %479, align 4, !tbaa !284
  %481 = insertelement <32 x float> undef, float %480, i32 0
  %482 = shufflevector <32 x float> %481, <32 x float> undef, <32 x i32> zeroinitializer
  %483 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %482, <32 x float> %439, <32 x float> %418)
  %484 = add nuw nsw i64 %436, 8192
  %485 = getelementptr inbounds [18432 x <32 x float>], [18432 x <32 x float>]* %5, i64 0, i64 0, i64 %484
  %486 = bitcast float* %485 to <32 x float>*
  %487 = load <32 x float>, <32 x float>* %486, align 16, !tbaa !290
  %488 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %446, <32 x float> %487, <32 x float> %440)
  %489 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %452, <32 x float> %487, <32 x float> %447)
  %490 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %458, <32 x float> %487, <32 x float> %453)
  %491 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %464, <32 x float> %487, <32 x float> %459)
  %492 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %470, <32 x float> %487, <32 x float> %465)
  %493 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %476, <32 x float> %487, <32 x float> %471)
  %494 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %482, <32 x float> %487, <32 x float> %477)
  %495 = add nsw i64 %429, 8
  %496 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %495
  %497 = load float, float* %496, align 8, !tbaa !284
  %498 = insertelement <32 x float> undef, float %497, i32 0
  %499 = shufflevector <32 x float> %498, <32 x float> undef, <32 x i32> zeroinitializer
  %500 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %499, <32 x float> %487, <32 x float> %483)
  %501 = add nuw nsw i64 %436, 16384
  %502 = getelementptr inbounds [18432 x <32 x float>], [18432 x <32 x float>]* %5, i64 0, i64 0, i64 %501
  %503 = bitcast float* %502 to <32 x float>*
  %504 = load <32 x float>, <32 x float>* %503, align 16, !tbaa !290
  %505 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %452, <32 x float> %504, <32 x float> %488)
  %506 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %458, <32 x float> %504, <32 x float> %489)
  %507 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %464, <32 x float> %504, <32 x float> %490)
  %508 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %470, <32 x float> %504, <32 x float> %491)
  %509 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %476, <32 x float> %504, <32 x float> %492)
  %510 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %482, <32 x float> %504, <32 x float> %493)
  %511 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %499, <32 x float> %504, <32 x float> %494)
  %512 = add nsw i64 %429, 9
  %513 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %512
  %514 = load float, float* %513, align 4, !tbaa !284
  %515 = insertelement <32 x float> undef, float %514, i32 0
  %516 = shufflevector <32 x float> %515, <32 x float> undef, <32 x i32> zeroinitializer
  %517 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %516, <32 x float> %504, <32 x float> %500)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %518 = add nuw nsw i32 %426, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 256
  br i1 %exitcond.1, label %for_end27.1, label %for_body26.1, !prof !34

for_end27.1:                                      ; preds = %for_body26.1
  %519 = add nuw nsw i64 %207, %201
  %520 = trunc i64 %519 to i32
  br label %for_body26.2

for_body26.2:                                     ; preds = %for_body26.2, %for_end27.1
  %indvars.iv.2 = phi i64 [ 0, %for_end27.1 ], [ %indvars.iv.next.2, %for_body26.2 ]
  %521 = phi <32 x float> [ %517, %for_end27.1 ], [ %620, %for_body26.2 ]
  %522 = phi <32 x float> [ %511, %for_end27.1 ], [ %614, %for_body26.2 ]
  %523 = phi <32 x float> [ %510, %for_end27.1 ], [ %613, %for_body26.2 ]
  %524 = phi <32 x float> [ %509, %for_end27.1 ], [ %612, %for_body26.2 ]
  %525 = phi <32 x float> [ %508, %for_end27.1 ], [ %611, %for_body26.2 ]
  %526 = phi <32 x float> [ %507, %for_end27.1 ], [ %610, %for_body26.2 ]
  %527 = phi <32 x float> [ %506, %for_end27.1 ], [ %609, %for_body26.2 ]
  %528 = phi <32 x float> [ %505, %for_end27.1 ], [ %608, %for_body26.2 ]
  %529 = phi i32 [ 0, %for_end27.1 ], [ %621, %for_body26.2 ]
  %530 = mul nuw nsw i64 %indvars.iv.2, 58
  %531 = mul nuw nsw i32 %529, 58
  %532 = add nsw i64 %519, %530
  %533 = add nsw i32 %531, %520
  %534 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %532
  %535 = load float, float* %534, align 8, !tbaa !284
  %536 = insertelement <32 x float> undef, float %535, i32 0
  %537 = shufflevector <32 x float> %536, <32 x float> undef, <32 x i32> zeroinitializer
  %538 = shl nsw i64 %indvars.iv.2, 5
  %539 = add nuw nsw i64 %202, %538
  %540 = getelementptr inbounds [18432 x <32 x float>], [18432 x <32 x float>]* %5, i64 0, i64 0, i64 %539
  %541 = bitcast float* %540 to <32 x float>*
  %542 = load <32 x float>, <32 x float>* %541, align 16, !tbaa !290
  %543 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %537, <32 x float> %542, <32 x float> %528)
  %544 = or i32 %533, 1
  %545 = sext i32 %544 to i64
  %546 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %545
  %547 = load float, float* %546, align 4, !tbaa !284
  %548 = insertelement <32 x float> undef, float %547, i32 0
  %549 = shufflevector <32 x float> %548, <32 x float> undef, <32 x i32> zeroinitializer
  %550 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %549, <32 x float> %542, <32 x float> %527)
  %551 = add nsw i64 %532, 2
  %552 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %551
  %553 = load float, float* %552, align 8, !tbaa !284
  %554 = insertelement <32 x float> undef, float %553, i32 0
  %555 = shufflevector <32 x float> %554, <32 x float> undef, <32 x i32> zeroinitializer
  %556 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %555, <32 x float> %542, <32 x float> %526)
  %557 = add nsw i64 %532, 3
  %558 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %557
  %559 = load float, float* %558, align 4, !tbaa !284
  %560 = insertelement <32 x float> undef, float %559, i32 0
  %561 = shufflevector <32 x float> %560, <32 x float> undef, <32 x i32> zeroinitializer
  %562 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %561, <32 x float> %542, <32 x float> %525)
  %563 = add nsw i64 %532, 4
  %564 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %563
  %565 = load float, float* %564, align 8, !tbaa !284
  %566 = insertelement <32 x float> undef, float %565, i32 0
  %567 = shufflevector <32 x float> %566, <32 x float> undef, <32 x i32> zeroinitializer
  %568 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %567, <32 x float> %542, <32 x float> %524)
  %569 = add nsw i64 %532, 5
  %570 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %569
  %571 = load float, float* %570, align 4, !tbaa !284
  %572 = insertelement <32 x float> undef, float %571, i32 0
  %573 = shufflevector <32 x float> %572, <32 x float> undef, <32 x i32> zeroinitializer
  %574 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %573, <32 x float> %542, <32 x float> %523)
  %575 = add nsw i64 %532, 6
  %576 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %575
  %577 = load float, float* %576, align 8, !tbaa !284
  %578 = insertelement <32 x float> undef, float %577, i32 0
  %579 = shufflevector <32 x float> %578, <32 x float> undef, <32 x i32> zeroinitializer
  %580 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %579, <32 x float> %542, <32 x float> %522)
  %581 = add nsw i64 %532, 7
  %582 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %581
  %583 = load float, float* %582, align 4, !tbaa !284
  %584 = insertelement <32 x float> undef, float %583, i32 0
  %585 = shufflevector <32 x float> %584, <32 x float> undef, <32 x i32> zeroinitializer
  %586 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %585, <32 x float> %542, <32 x float> %521)
  %587 = add nuw nsw i64 %539, 8192
  %588 = getelementptr inbounds [18432 x <32 x float>], [18432 x <32 x float>]* %5, i64 0, i64 0, i64 %587
  %589 = bitcast float* %588 to <32 x float>*
  %590 = load <32 x float>, <32 x float>* %589, align 16, !tbaa !290
  %591 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %549, <32 x float> %590, <32 x float> %543)
  %592 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %555, <32 x float> %590, <32 x float> %550)
  %593 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %561, <32 x float> %590, <32 x float> %556)
  %594 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %567, <32 x float> %590, <32 x float> %562)
  %595 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %573, <32 x float> %590, <32 x float> %568)
  %596 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %579, <32 x float> %590, <32 x float> %574)
  %597 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %585, <32 x float> %590, <32 x float> %580)
  %598 = add nsw i64 %532, 8
  %599 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %598
  %600 = load float, float* %599, align 8, !tbaa !284
  %601 = insertelement <32 x float> undef, float %600, i32 0
  %602 = shufflevector <32 x float> %601, <32 x float> undef, <32 x i32> zeroinitializer
  %603 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %602, <32 x float> %590, <32 x float> %586)
  %604 = add nuw nsw i64 %539, 16384
  %605 = getelementptr inbounds [18432 x <32 x float>], [18432 x <32 x float>]* %5, i64 0, i64 0, i64 %604
  %606 = bitcast float* %605 to <32 x float>*
  %607 = load <32 x float>, <32 x float>* %606, align 16, !tbaa !290
  %608 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %555, <32 x float> %607, <32 x float> %591)
  %609 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %561, <32 x float> %607, <32 x float> %592)
  %610 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %567, <32 x float> %607, <32 x float> %593)
  %611 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %573, <32 x float> %607, <32 x float> %594)
  %612 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %579, <32 x float> %607, <32 x float> %595)
  %613 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %585, <32 x float> %607, <32 x float> %596)
  %614 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %602, <32 x float> %607, <32 x float> %597)
  %615 = add nsw i64 %532, 9
  %616 = getelementptr inbounds [861184 x float], [861184 x float]* %6, i64 0, i64 %615
  %617 = load float, float* %616, align 4, !tbaa !284
  %618 = insertelement <32 x float> undef, float %617, i32 0
  %619 = shufflevector <32 x float> %618, <32 x float> undef, <32 x i32> zeroinitializer
  %620 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %619, <32 x float> %607, <32 x float> %603)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %621 = add nuw nsw i32 %529, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 256
  br i1 %exitcond.2, label %for_end27.2, label %for_body26.2, !prof !34

for_end27.2:                                      ; preds = %for_body26.2
  store <32 x float> %608, <32 x float>* %.sub, align 16, !tbaa !299
  store <32 x float> %609, <32 x float>* %37, align 16, !tbaa !299
  store <32 x float> %610, <32 x float>* %39, align 16, !tbaa !299
  store <32 x float> %611, <32 x float>* %41, align 16, !tbaa !299
  store <32 x float> %612, <32 x float>* %43, align 16, !tbaa !299
  store <32 x float> %613, <32 x float>* %45, align 16, !tbaa !299
  store <32 x float> %614, <32 x float>* %47, align 16, !tbaa !299
  store <32 x float> %620, <32 x float>* %49, align 16, !tbaa !299
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep53, i8* nonnull align 16 %26, i64 1024, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond54 = icmp eq i64 %indvar.next, 7
  br i1 %exitcond54, label %for_begin31.preheader, label %for_body20, !prof !34
}

; Function Attrs: nounwind readnone speculatable
declare <32 x float> @llvm.fmuladd.v32f32(<32 x float>, <32 x float>, <32 x float>) #4

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
define private fastcc void @fused_nn_conv2d_2_compute_(i8* noalias nocapture readonly, i8* noalias readonly, i8* noalias) unnamed_addr #3 {
entry:
  %3 = alloca [7 x <32 x float>], align 128
  %4 = alloca [28 x <32 x float>], align 16
  %5 = alloca [36864 x <32 x float>], align 16
  %6 = alloca [230400 x float], align 16
  %7 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar69 = phi i64 [ 0, %entry ], [ %indvar.next70, %for_end3 ]
  %8 = mul nuw nsw i64 %indvar69, 7680
  %9 = trunc i64 %indvar69 to i32
  %10 = add i32 %9, -1
  %11 = icmp ult i32 %10, 28
  %12 = mul nuw nsw i64 %indvar69, 28
  %13 = add nsw i64 %12, -29
  br i1 %11, label %if_end.us.us.29, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep76 = getelementptr [230400 x float], [230400 x float]* %6, i64 0, i64 %8
  %scevgep7677 = bitcast float* %scevgep76 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep7677, i8 0, i64 30720, i1 false)
  br label %for_end3

for_begin7.preheader:                             ; preds = %for_end3
  %14 = bitcast [7 x <32 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0
  %15 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_end3:                                         ; preds = %if_end.us.us.29, %for_begin4.preheader.preheader
  %indvar.next70 = add nuw nsw i64 %indvar69, 1
  %exitcond85 = icmp eq i64 %indvar.next70, 30
  br i1 %exitcond85, label %for_begin7.preheader, label %for_begin1.preheader, !prof !34

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %indvars.iv63 = phi i64 [ 0, %for_begin7.preheader ], [ %indvars.iv.next64, %for_end12 ]
  %16 = mul nuw nsw i64 %indvars.iv63, 24576
  %17 = trunc i64 %indvars.iv63 to i32
  %18 = urem i32 %17, 3
  %19 = mul nuw nsw i32 %18, 3
  %20 = udiv i32 %17, 3
  %21 = mul nsw i32 %20, 73728
  %22 = or i32 %19, %21
  %23 = zext i32 %22 to i64
  br label %for_begin13.preheader

for_begin16.preheader:                            ; preds = %for_end12
  %24 = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0, i64 32
  %25 = bitcast float* %24 to <32 x float>*
  %26 = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0, i64 64
  %27 = bitcast float* %26 to <32 x float>*
  %28 = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0, i64 96
  %29 = bitcast float* %28 to <32 x float>*
  %30 = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0, i64 128
  %31 = bitcast float* %30 to <32 x float>*
  %32 = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0, i64 160
  %33 = bitcast float* %32 to <32 x float>*
  %34 = getelementptr inbounds [7 x <32 x float>], [7 x <32 x float>]* %3, i64 0, i64 0, i64 192
  %35 = bitcast float* %34 to <32 x float>*
  %36 = bitcast i8* %2 to float*
  %37 = bitcast [7 x <32 x float>]* %3 to i8*
  br label %for_begin19.preheader

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvars.iv60 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next61, %for_end15 ]
  %38 = shl i64 %indvars.iv60, 13
  %39 = add nuw nsw i64 %38, %16
  %40 = add nuw nsw i64 %indvars.iv60, %23
  br label %for_body14

for_end12:                                        ; preds = %for_end15
  %indvars.iv.next64 = add nuw nsw i64 %indvars.iv63, 1
  %exitcond65 = icmp eq i64 %indvars.iv.next64, 48
  br i1 %exitcond65, label %for_begin16.preheader, label %for_begin10.preheader, !prof !34

for_body14:                                       ; preds = %for_body14, %for_begin13.preheader
  %indvars.iv57 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next58, %for_body14 ]
  %41 = shl i64 %indvars.iv57, 5
  %42 = add nuw nsw i64 %39, %41
  %43 = mul nuw nsw i64 %indvars.iv57, 9
  %44 = add nuw nsw i64 %40, %43
  %45 = add nuw nsw i64 %44, 2304
  %46 = add nuw nsw i64 %44, 4608
  %47 = add nuw nsw i64 %44, 6912
  %48 = add nuw nsw i64 %44, 9216
  %49 = add nuw nsw i64 %44, 11520
  %50 = add nuw nsw i64 %44, 13824
  %51 = add nuw nsw i64 %44, 16128
  %52 = add nuw nsw i64 %44, 18432
  %53 = add nuw nsw i64 %44, 20736
  %54 = add nuw nsw i64 %44, 23040
  %55 = add nuw nsw i64 %44, 25344
  %56 = add nuw nsw i64 %44, 27648
  %57 = add nuw nsw i64 %44, 29952
  %58 = add nuw nsw i64 %44, 32256
  %59 = add nuw nsw i64 %44, 34560
  %60 = add nuw nsw i64 %44, 36864
  %61 = add nuw nsw i64 %44, 39168
  %62 = add nuw nsw i64 %44, 41472
  %63 = add nuw nsw i64 %44, 43776
  %64 = add nuw nsw i64 %44, 46080
  %65 = add nuw nsw i64 %44, 48384
  %66 = add nuw nsw i64 %44, 50688
  %67 = add nuw nsw i64 %44, 52992
  %68 = add nuw nsw i64 %44, 55296
  %69 = add nuw nsw i64 %44, 57600
  %70 = add nuw nsw i64 %44, 59904
  %71 = add nuw nsw i64 %44, 62208
  %72 = add nuw nsw i64 %44, 64512
  %73 = add nuw nsw i64 %44, 66816
  %74 = add nuw nsw i64 %44, 69120
  %75 = add nuw nsw i64 %44, 71424
  %76 = getelementptr inbounds float, float* %15, i64 %44
  %77 = load float, float* %76, align 4, !tbaa !308
  %78 = insertelement <32 x float> undef, float %77, i32 0
  %79 = getelementptr inbounds float, float* %15, i64 %45
  %80 = load float, float* %79, align 4, !tbaa !308
  %81 = insertelement <32 x float> %78, float %80, i32 1
  %82 = getelementptr inbounds float, float* %15, i64 %46
  %83 = load float, float* %82, align 4, !tbaa !308
  %84 = insertelement <32 x float> %81, float %83, i32 2
  %85 = getelementptr inbounds float, float* %15, i64 %47
  %86 = load float, float* %85, align 4, !tbaa !308
  %87 = insertelement <32 x float> %84, float %86, i32 3
  %88 = getelementptr inbounds float, float* %15, i64 %48
  %89 = load float, float* %88, align 4, !tbaa !308
  %90 = insertelement <32 x float> %87, float %89, i32 4
  %91 = getelementptr inbounds float, float* %15, i64 %49
  %92 = load float, float* %91, align 4, !tbaa !308
  %93 = insertelement <32 x float> %90, float %92, i32 5
  %94 = getelementptr inbounds float, float* %15, i64 %50
  %95 = load float, float* %94, align 4, !tbaa !308
  %96 = insertelement <32 x float> %93, float %95, i32 6
  %97 = getelementptr inbounds float, float* %15, i64 %51
  %98 = load float, float* %97, align 4, !tbaa !308
  %99 = insertelement <32 x float> %96, float %98, i32 7
  %100 = getelementptr inbounds float, float* %15, i64 %52
  %101 = load float, float* %100, align 4, !tbaa !308
  %102 = insertelement <32 x float> %99, float %101, i32 8
  %103 = getelementptr inbounds float, float* %15, i64 %53
  %104 = load float, float* %103, align 4, !tbaa !308
  %105 = insertelement <32 x float> %102, float %104, i32 9
  %106 = getelementptr inbounds float, float* %15, i64 %54
  %107 = load float, float* %106, align 4, !tbaa !308
  %108 = insertelement <32 x float> %105, float %107, i32 10
  %109 = getelementptr inbounds float, float* %15, i64 %55
  %110 = load float, float* %109, align 4, !tbaa !308
  %111 = insertelement <32 x float> %108, float %110, i32 11
  %112 = getelementptr inbounds float, float* %15, i64 %56
  %113 = load float, float* %112, align 4, !tbaa !308
  %114 = insertelement <32 x float> %111, float %113, i32 12
  %115 = getelementptr inbounds float, float* %15, i64 %57
  %116 = load float, float* %115, align 4, !tbaa !308
  %117 = insertelement <32 x float> %114, float %116, i32 13
  %118 = getelementptr inbounds float, float* %15, i64 %58
  %119 = load float, float* %118, align 4, !tbaa !308
  %120 = insertelement <32 x float> %117, float %119, i32 14
  %121 = getelementptr inbounds float, float* %15, i64 %59
  %122 = load float, float* %121, align 4, !tbaa !308
  %123 = insertelement <32 x float> %120, float %122, i32 15
  %124 = getelementptr inbounds float, float* %15, i64 %60
  %125 = load float, float* %124, align 4, !tbaa !308
  %126 = insertelement <32 x float> %123, float %125, i32 16
  %127 = getelementptr inbounds float, float* %15, i64 %61
  %128 = load float, float* %127, align 4, !tbaa !308
  %129 = insertelement <32 x float> %126, float %128, i32 17
  %130 = getelementptr inbounds float, float* %15, i64 %62
  %131 = load float, float* %130, align 4, !tbaa !308
  %132 = insertelement <32 x float> %129, float %131, i32 18
  %133 = getelementptr inbounds float, float* %15, i64 %63
  %134 = load float, float* %133, align 4, !tbaa !308
  %135 = insertelement <32 x float> %132, float %134, i32 19
  %136 = getelementptr inbounds float, float* %15, i64 %64
  %137 = load float, float* %136, align 4, !tbaa !308
  %138 = insertelement <32 x float> %135, float %137, i32 20
  %139 = getelementptr inbounds float, float* %15, i64 %65
  %140 = load float, float* %139, align 4, !tbaa !308
  %141 = insertelement <32 x float> %138, float %140, i32 21
  %142 = getelementptr inbounds float, float* %15, i64 %66
  %143 = load float, float* %142, align 4, !tbaa !308
  %144 = insertelement <32 x float> %141, float %143, i32 22
  %145 = getelementptr inbounds float, float* %15, i64 %67
  %146 = load float, float* %145, align 4, !tbaa !308
  %147 = insertelement <32 x float> %144, float %146, i32 23
  %148 = getelementptr inbounds float, float* %15, i64 %68
  %149 = load float, float* %148, align 4, !tbaa !308
  %150 = insertelement <32 x float> %147, float %149, i32 24
  %151 = getelementptr inbounds float, float* %15, i64 %69
  %152 = load float, float* %151, align 4, !tbaa !308
  %153 = insertelement <32 x float> %150, float %152, i32 25
  %154 = getelementptr inbounds float, float* %15, i64 %70
  %155 = load float, float* %154, align 4, !tbaa !308
  %156 = insertelement <32 x float> %153, float %155, i32 26
  %157 = getelementptr inbounds float, float* %15, i64 %71
  %158 = load float, float* %157, align 4, !tbaa !308
  %159 = insertelement <32 x float> %156, float %158, i32 27
  %160 = getelementptr inbounds float, float* %15, i64 %72
  %161 = load float, float* %160, align 4, !tbaa !308
  %162 = insertelement <32 x float> %159, float %161, i32 28
  %163 = getelementptr inbounds float, float* %15, i64 %73
  %164 = load float, float* %163, align 4, !tbaa !308
  %165 = insertelement <32 x float> %162, float %164, i32 29
  %166 = getelementptr inbounds float, float* %15, i64 %74
  %167 = load float, float* %166, align 4, !tbaa !308
  %168 = insertelement <32 x float> %165, float %167, i32 30
  %169 = getelementptr inbounds float, float* %15, i64 %75
  %170 = load float, float* %169, align 4, !tbaa !308
  %171 = insertelement <32 x float> %168, float %170, i32 31
  %172 = getelementptr inbounds [36864 x <32 x float>], [36864 x <32 x float>]* %5, i64 0, i64 0, i64 %42
  %173 = bitcast float* %172 to <32 x float>*
  store <32 x float> %171, <32 x float>* %173, align 16, !tbaa !311
  %indvars.iv.next58 = add nuw nsw i64 %indvars.iv57, 1
  %exitcond59 = icmp eq i64 %indvars.iv.next58, 256
  br i1 %exitcond59, label %for_end15, label %for_body14, !prof !34

for_end15:                                        ; preds = %for_body14
  %indvars.iv.next61 = add nuw nsw i64 %indvars.iv60, 1
  %exitcond62 = icmp eq i64 %indvars.iv.next61, 3
  br i1 %exitcond62, label %for_end12, label %for_begin13.preheader, !prof !34

for_begin19.preheader:                            ; preds = %for_end33, %for_begin16.preheader
  %174 = phi i32 [ 0, %for_begin16.preheader ], [ %286, %for_end33 ]
  %175 = urem i32 %174, 28
  %176 = udiv i32 %174, 28
  %177 = mul nsw i32 %176, 73728
  %178 = zext i32 %177 to i64
  %179 = mul nuw nsw i32 %175, 7680
  %180 = zext i32 %179 to i64
  %181 = mul nuw nsw i32 %175, 7680
  %182 = add nuw nsw i32 %181, 7680
  %183 = zext i32 %182 to i64
  %184 = add nuw nsw i64 %178, 24576
  %185 = mul nuw nsw i32 %175, 7680
  %186 = add nuw nsw i32 %185, 15360
  %187 = zext i32 %186 to i64
  %188 = add nuw nsw i64 %178, 49152
  br label %for_body20

for_end18:                                        ; preds = %for_end33
  ret void

for_begin31.preheader:                            ; preds = %for_end27.2
  %189 = mul nuw nsw i32 %175, 28
  %190 = mul nsw i32 %176, 25088
  %191 = add nuw nsw i32 %190, %189
  %192 = zext i32 %191 to i64
  br label %for_begin34.preheader

for_body20:                                       ; preds = %for_end27.2, %for_begin19.preheader
  %indvar = phi i64 [ 0, %for_begin19.preheader ], [ %indvar.next, %for_end27.2 ]
  %193 = mul nuw nsw i64 %indvar, 7
  %scevgep = getelementptr [28 x <32 x float>], [28 x <32 x float>]* %4, i64 0, i64 %193
  %scevgep48 = bitcast <32 x float>* %scevgep to i8*
  %194 = add nuw nsw i64 %193, %180
  call void @llvm.memset.p0i8.i64(i8* nonnull align 128 %37, i8 0, i64 896, i1 false)
  br label %for_body26

for_body26:                                       ; preds = %for_body26, %for_body20
  %indvars.iv = phi i64 [ 0, %for_body20 ], [ %indvars.iv.next, %for_body26 ]
  %195 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %281, %for_body26 ]
  %196 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %275, %for_body26 ]
  %197 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %274, %for_body26 ]
  %198 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %273, %for_body26 ]
  %199 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %272, %for_body26 ]
  %200 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %271, %for_body26 ]
  %201 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %270, %for_body26 ]
  %202 = mul nuw nsw i64 %indvars.iv, 30
  %203 = add nuw nsw i64 %194, %202
  %204 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %203
  %205 = load float, float* %204, align 4, !tbaa !314
  %206 = insertelement <32 x float> undef, float %205, i32 0
  %207 = shufflevector <32 x float> %206, <32 x float> undef, <32 x i32> zeroinitializer
  %208 = shl nsw i64 %indvars.iv, 5
  %209 = add nuw nsw i64 %208, %178
  %210 = getelementptr inbounds [36864 x <32 x float>], [36864 x <32 x float>]* %5, i64 0, i64 0, i64 %209
  %211 = bitcast float* %210 to <32 x float>*
  %212 = load <32 x float>, <32 x float>* %211, align 16, !tbaa !311
  %213 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %207, <32 x float> %212, <32 x float> %201)
  %214 = add nuw nsw i64 %203, 1
  %215 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %214
  %216 = load float, float* %215, align 4, !tbaa !314
  %217 = insertelement <32 x float> undef, float %216, i32 0
  %218 = shufflevector <32 x float> %217, <32 x float> undef, <32 x i32> zeroinitializer
  %219 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %218, <32 x float> %212, <32 x float> %200)
  %220 = add nuw nsw i64 %203, 2
  %221 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %220
  %222 = load float, float* %221, align 4, !tbaa !314
  %223 = insertelement <32 x float> undef, float %222, i32 0
  %224 = shufflevector <32 x float> %223, <32 x float> undef, <32 x i32> zeroinitializer
  %225 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %224, <32 x float> %212, <32 x float> %199)
  %226 = add nuw nsw i64 %203, 3
  %227 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %226
  %228 = load float, float* %227, align 4, !tbaa !314
  %229 = insertelement <32 x float> undef, float %228, i32 0
  %230 = shufflevector <32 x float> %229, <32 x float> undef, <32 x i32> zeroinitializer
  %231 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %230, <32 x float> %212, <32 x float> %198)
  %232 = add nuw nsw i64 %203, 4
  %233 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %232
  %234 = load float, float* %233, align 4, !tbaa !314
  %235 = insertelement <32 x float> undef, float %234, i32 0
  %236 = shufflevector <32 x float> %235, <32 x float> undef, <32 x i32> zeroinitializer
  %237 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %236, <32 x float> %212, <32 x float> %197)
  %238 = add nuw nsw i64 %203, 5
  %239 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %238
  %240 = load float, float* %239, align 4, !tbaa !314
  %241 = insertelement <32 x float> undef, float %240, i32 0
  %242 = shufflevector <32 x float> %241, <32 x float> undef, <32 x i32> zeroinitializer
  %243 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %242, <32 x float> %212, <32 x float> %196)
  %244 = add nuw nsw i64 %203, 6
  %245 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %244
  %246 = load float, float* %245, align 4, !tbaa !314
  %247 = insertelement <32 x float> undef, float %246, i32 0
  %248 = shufflevector <32 x float> %247, <32 x float> undef, <32 x i32> zeroinitializer
  %249 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %248, <32 x float> %212, <32 x float> %195)
  %250 = add nuw nsw i64 %209, 8192
  %251 = getelementptr inbounds [36864 x <32 x float>], [36864 x <32 x float>]* %5, i64 0, i64 0, i64 %250
  %252 = bitcast float* %251 to <32 x float>*
  %253 = load <32 x float>, <32 x float>* %252, align 16, !tbaa !311
  %254 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %218, <32 x float> %253, <32 x float> %213)
  %255 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %224, <32 x float> %253, <32 x float> %219)
  %256 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %230, <32 x float> %253, <32 x float> %225)
  %257 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %236, <32 x float> %253, <32 x float> %231)
  %258 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %242, <32 x float> %253, <32 x float> %237)
  %259 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %248, <32 x float> %253, <32 x float> %243)
  %260 = add nuw nsw i64 %203, 7
  %261 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %260
  %262 = load float, float* %261, align 4, !tbaa !314
  %263 = insertelement <32 x float> undef, float %262, i32 0
  %264 = shufflevector <32 x float> %263, <32 x float> undef, <32 x i32> zeroinitializer
  %265 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %264, <32 x float> %253, <32 x float> %249)
  %266 = add nuw nsw i64 %209, 16384
  %267 = getelementptr inbounds [36864 x <32 x float>], [36864 x <32 x float>]* %5, i64 0, i64 0, i64 %266
  %268 = bitcast float* %267 to <32 x float>*
  %269 = load <32 x float>, <32 x float>* %268, align 16, !tbaa !311
  %270 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %224, <32 x float> %269, <32 x float> %254)
  %271 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %230, <32 x float> %269, <32 x float> %255)
  %272 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %236, <32 x float> %269, <32 x float> %256)
  %273 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %242, <32 x float> %269, <32 x float> %257)
  %274 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %248, <32 x float> %269, <32 x float> %258)
  %275 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %264, <32 x float> %269, <32 x float> %259)
  %276 = add nuw nsw i64 %203, 8
  %277 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %276
  %278 = load float, float* %277, align 4, !tbaa !314
  %279 = insertelement <32 x float> undef, float %278, i32 0
  %280 = shufflevector <32 x float> %279, <32 x float> undef, <32 x i32> zeroinitializer
  %281 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %280, <32 x float> %269, <32 x float> %265)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 256
  br i1 %exitcond, label %for_end27, label %for_body26, !prof !34

for_end27:                                        ; preds = %for_body26
  %282 = add nuw nsw i64 %193, %183
  br label %for_body26.1

for_begin34.preheader:                            ; preds = %for_end36, %for_begin31.preheader
  %indvars.iv53 = phi i64 [ 0, %for_begin31.preheader ], [ %indvars.iv.next54, %for_end36 ]
  %283 = mul nuw nsw i64 %indvars.iv53, 7
  %284 = add nuw nsw i64 %283, %192
  %285 = mul nuw nsw i64 %indvars.iv53, 224
  br label %for_body35

for_end33:                                        ; preds = %for_end36
  %286 = add nuw nsw i32 %174, 1
  %exitcond56 = icmp eq i32 %286, 448
  br i1 %exitcond56, label %for_end18, label %for_begin19.preheader, !prof !34

for_body35:                                       ; preds = %for_body35, %for_begin34.preheader
  %indvars.iv50 = phi i64 [ 0, %for_begin34.preheader ], [ %indvars.iv.next51, %for_body35 ]
  %287 = add nuw nsw i64 %284, %indvars.iv50
  %288 = add nuw nsw i64 %287, 784
  %289 = add nuw nsw i64 %287, 1568
  %290 = add nuw nsw i64 %287, 2352
  %291 = add nuw nsw i64 %287, 3136
  %292 = add nuw nsw i64 %287, 3920
  %293 = add nuw nsw i64 %287, 4704
  %294 = add nuw nsw i64 %287, 5488
  %295 = add nuw nsw i64 %287, 6272
  %296 = add nuw nsw i64 %287, 7056
  %297 = add nuw nsw i64 %287, 7840
  %298 = add nuw nsw i64 %287, 8624
  %299 = add nuw nsw i64 %287, 9408
  %300 = add nuw nsw i64 %287, 10192
  %301 = add nuw nsw i64 %287, 10976
  %302 = add nuw nsw i64 %287, 11760
  %303 = add nuw nsw i64 %287, 12544
  %304 = add nuw nsw i64 %287, 13328
  %305 = add nuw nsw i64 %287, 14112
  %306 = add nuw nsw i64 %287, 14896
  %307 = add nuw nsw i64 %287, 15680
  %308 = add nuw nsw i64 %287, 16464
  %309 = add nuw nsw i64 %287, 17248
  %310 = add nuw nsw i64 %287, 18032
  %311 = add nuw nsw i64 %287, 18816
  %312 = add nuw nsw i64 %287, 19600
  %313 = add nuw nsw i64 %287, 20384
  %314 = add nuw nsw i64 %287, 21168
  %315 = add nuw nsw i64 %287, 21952
  %316 = add nuw nsw i64 %287, 22736
  %317 = add nuw nsw i64 %287, 23520
  %318 = add nuw nsw i64 %287, 24304
  %319 = shl i64 %indvars.iv50, 5
  %320 = add nuw nsw i64 %319, %285
  %321 = getelementptr inbounds [28 x <32 x float>], [28 x <32 x float>]* %4, i64 0, i64 0, i64 %320
  %322 = bitcast float* %321 to <32 x float>*
  %323 = load <32 x float>, <32 x float>* %322, align 16, !tbaa !317
  %324 = getelementptr inbounds float, float* %36, i64 %287
  %325 = extractelement <32 x float> %323, i64 0
  store float %325, float* %324, align 4, !tbaa !320
  %326 = getelementptr inbounds float, float* %36, i64 %288
  %327 = extractelement <32 x float> %323, i64 1
  store float %327, float* %326, align 4, !tbaa !320
  %328 = getelementptr inbounds float, float* %36, i64 %289
  %329 = extractelement <32 x float> %323, i64 2
  store float %329, float* %328, align 4, !tbaa !320
  %330 = getelementptr inbounds float, float* %36, i64 %290
  %331 = extractelement <32 x float> %323, i64 3
  store float %331, float* %330, align 4, !tbaa !320
  %332 = getelementptr inbounds float, float* %36, i64 %291
  %333 = extractelement <32 x float> %323, i64 4
  store float %333, float* %332, align 4, !tbaa !320
  %334 = getelementptr inbounds float, float* %36, i64 %292
  %335 = extractelement <32 x float> %323, i64 5
  store float %335, float* %334, align 4, !tbaa !320
  %336 = getelementptr inbounds float, float* %36, i64 %293
  %337 = extractelement <32 x float> %323, i64 6
  store float %337, float* %336, align 4, !tbaa !320
  %338 = getelementptr inbounds float, float* %36, i64 %294
  %339 = extractelement <32 x float> %323, i64 7
  store float %339, float* %338, align 4, !tbaa !320
  %340 = getelementptr inbounds float, float* %36, i64 %295
  %341 = extractelement <32 x float> %323, i64 8
  store float %341, float* %340, align 4, !tbaa !320
  %342 = getelementptr inbounds float, float* %36, i64 %296
  %343 = extractelement <32 x float> %323, i64 9
  store float %343, float* %342, align 4, !tbaa !320
  %344 = getelementptr inbounds float, float* %36, i64 %297
  %345 = extractelement <32 x float> %323, i64 10
  store float %345, float* %344, align 4, !tbaa !320
  %346 = getelementptr inbounds float, float* %36, i64 %298
  %347 = extractelement <32 x float> %323, i64 11
  store float %347, float* %346, align 4, !tbaa !320
  %348 = getelementptr inbounds float, float* %36, i64 %299
  %349 = extractelement <32 x float> %323, i64 12
  store float %349, float* %348, align 4, !tbaa !320
  %350 = getelementptr inbounds float, float* %36, i64 %300
  %351 = extractelement <32 x float> %323, i64 13
  store float %351, float* %350, align 4, !tbaa !320
  %352 = getelementptr inbounds float, float* %36, i64 %301
  %353 = extractelement <32 x float> %323, i64 14
  store float %353, float* %352, align 4, !tbaa !320
  %354 = getelementptr inbounds float, float* %36, i64 %302
  %355 = extractelement <32 x float> %323, i64 15
  store float %355, float* %354, align 4, !tbaa !320
  %356 = getelementptr inbounds float, float* %36, i64 %303
  %357 = extractelement <32 x float> %323, i64 16
  store float %357, float* %356, align 4, !tbaa !320
  %358 = getelementptr inbounds float, float* %36, i64 %304
  %359 = extractelement <32 x float> %323, i64 17
  store float %359, float* %358, align 4, !tbaa !320
  %360 = getelementptr inbounds float, float* %36, i64 %305
  %361 = extractelement <32 x float> %323, i64 18
  store float %361, float* %360, align 4, !tbaa !320
  %362 = getelementptr inbounds float, float* %36, i64 %306
  %363 = extractelement <32 x float> %323, i64 19
  store float %363, float* %362, align 4, !tbaa !320
  %364 = getelementptr inbounds float, float* %36, i64 %307
  %365 = extractelement <32 x float> %323, i64 20
  store float %365, float* %364, align 4, !tbaa !320
  %366 = getelementptr inbounds float, float* %36, i64 %308
  %367 = extractelement <32 x float> %323, i64 21
  store float %367, float* %366, align 4, !tbaa !320
  %368 = getelementptr inbounds float, float* %36, i64 %309
  %369 = extractelement <32 x float> %323, i64 22
  store float %369, float* %368, align 4, !tbaa !320
  %370 = getelementptr inbounds float, float* %36, i64 %310
  %371 = extractelement <32 x float> %323, i64 23
  store float %371, float* %370, align 4, !tbaa !320
  %372 = getelementptr inbounds float, float* %36, i64 %311
  %373 = extractelement <32 x float> %323, i64 24
  store float %373, float* %372, align 4, !tbaa !320
  %374 = getelementptr inbounds float, float* %36, i64 %312
  %375 = extractelement <32 x float> %323, i64 25
  store float %375, float* %374, align 4, !tbaa !320
  %376 = getelementptr inbounds float, float* %36, i64 %313
  %377 = extractelement <32 x float> %323, i64 26
  store float %377, float* %376, align 4, !tbaa !320
  %378 = getelementptr inbounds float, float* %36, i64 %314
  %379 = extractelement <32 x float> %323, i64 27
  store float %379, float* %378, align 4, !tbaa !320
  %380 = getelementptr inbounds float, float* %36, i64 %315
  %381 = extractelement <32 x float> %323, i64 28
  store float %381, float* %380, align 4, !tbaa !320
  %382 = getelementptr inbounds float, float* %36, i64 %316
  %383 = extractelement <32 x float> %323, i64 29
  store float %383, float* %382, align 4, !tbaa !320
  %384 = getelementptr inbounds float, float* %36, i64 %317
  %385 = extractelement <32 x float> %323, i64 30
  store float %385, float* %384, align 4, !tbaa !320
  %386 = getelementptr inbounds float, float* %36, i64 %318
  %387 = extractelement <32 x float> %323, i64 31
  store float %387, float* %386, align 4, !tbaa !320
  %indvars.iv.next51 = add nuw nsw i64 %indvars.iv50, 1
  %exitcond52 = icmp eq i64 %indvars.iv.next51, 7
  br i1 %exitcond52, label %for_end36, label %for_body35, !prof !34

for_end36:                                        ; preds = %for_body35
  %indvars.iv.next54 = add nuw nsw i64 %indvars.iv53, 1
  %exitcond55 = icmp eq i64 %indvars.iv.next54, 4
  br i1 %exitcond55, label %for_end33, label %for_begin34.preheader, !prof !34

for_body26.1:                                     ; preds = %for_body26.1, %for_end27
  %indvars.iv.1 = phi i64 [ 0, %for_end27 ], [ %indvars.iv.next.1, %for_body26.1 ]
  %388 = phi <32 x float> [ %281, %for_end27 ], [ %474, %for_body26.1 ]
  %389 = phi <32 x float> [ %275, %for_end27 ], [ %468, %for_body26.1 ]
  %390 = phi <32 x float> [ %274, %for_end27 ], [ %467, %for_body26.1 ]
  %391 = phi <32 x float> [ %273, %for_end27 ], [ %466, %for_body26.1 ]
  %392 = phi <32 x float> [ %272, %for_end27 ], [ %465, %for_body26.1 ]
  %393 = phi <32 x float> [ %271, %for_end27 ], [ %464, %for_body26.1 ]
  %394 = phi <32 x float> [ %270, %for_end27 ], [ %463, %for_body26.1 ]
  %395 = mul nuw nsw i64 %indvars.iv.1, 30
  %396 = add nuw nsw i64 %282, %395
  %397 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %396
  %398 = load float, float* %397, align 4, !tbaa !314
  %399 = insertelement <32 x float> undef, float %398, i32 0
  %400 = shufflevector <32 x float> %399, <32 x float> undef, <32 x i32> zeroinitializer
  %401 = shl nsw i64 %indvars.iv.1, 5
  %402 = add nuw nsw i64 %184, %401
  %403 = getelementptr inbounds [36864 x <32 x float>], [36864 x <32 x float>]* %5, i64 0, i64 0, i64 %402
  %404 = bitcast float* %403 to <32 x float>*
  %405 = load <32 x float>, <32 x float>* %404, align 16, !tbaa !311
  %406 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %400, <32 x float> %405, <32 x float> %394)
  %407 = add nuw nsw i64 %396, 1
  %408 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %407
  %409 = load float, float* %408, align 4, !tbaa !314
  %410 = insertelement <32 x float> undef, float %409, i32 0
  %411 = shufflevector <32 x float> %410, <32 x float> undef, <32 x i32> zeroinitializer
  %412 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %411, <32 x float> %405, <32 x float> %393)
  %413 = add nuw nsw i64 %396, 2
  %414 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %413
  %415 = load float, float* %414, align 4, !tbaa !314
  %416 = insertelement <32 x float> undef, float %415, i32 0
  %417 = shufflevector <32 x float> %416, <32 x float> undef, <32 x i32> zeroinitializer
  %418 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %417, <32 x float> %405, <32 x float> %392)
  %419 = add nuw nsw i64 %396, 3
  %420 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %419
  %421 = load float, float* %420, align 4, !tbaa !314
  %422 = insertelement <32 x float> undef, float %421, i32 0
  %423 = shufflevector <32 x float> %422, <32 x float> undef, <32 x i32> zeroinitializer
  %424 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %423, <32 x float> %405, <32 x float> %391)
  %425 = add nuw nsw i64 %396, 4
  %426 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %425
  %427 = load float, float* %426, align 4, !tbaa !314
  %428 = insertelement <32 x float> undef, float %427, i32 0
  %429 = shufflevector <32 x float> %428, <32 x float> undef, <32 x i32> zeroinitializer
  %430 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %429, <32 x float> %405, <32 x float> %390)
  %431 = add nuw nsw i64 %396, 5
  %432 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %431
  %433 = load float, float* %432, align 4, !tbaa !314
  %434 = insertelement <32 x float> undef, float %433, i32 0
  %435 = shufflevector <32 x float> %434, <32 x float> undef, <32 x i32> zeroinitializer
  %436 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %435, <32 x float> %405, <32 x float> %389)
  %437 = add nuw nsw i64 %396, 6
  %438 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %437
  %439 = load float, float* %438, align 4, !tbaa !314
  %440 = insertelement <32 x float> undef, float %439, i32 0
  %441 = shufflevector <32 x float> %440, <32 x float> undef, <32 x i32> zeroinitializer
  %442 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %441, <32 x float> %405, <32 x float> %388)
  %443 = add nuw nsw i64 %402, 8192
  %444 = getelementptr inbounds [36864 x <32 x float>], [36864 x <32 x float>]* %5, i64 0, i64 0, i64 %443
  %445 = bitcast float* %444 to <32 x float>*
  %446 = load <32 x float>, <32 x float>* %445, align 16, !tbaa !311
  %447 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %411, <32 x float> %446, <32 x float> %406)
  %448 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %417, <32 x float> %446, <32 x float> %412)
  %449 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %423, <32 x float> %446, <32 x float> %418)
  %450 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %429, <32 x float> %446, <32 x float> %424)
  %451 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %435, <32 x float> %446, <32 x float> %430)
  %452 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %441, <32 x float> %446, <32 x float> %436)
  %453 = add nuw nsw i64 %396, 7
  %454 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %453
  %455 = load float, float* %454, align 4, !tbaa !314
  %456 = insertelement <32 x float> undef, float %455, i32 0
  %457 = shufflevector <32 x float> %456, <32 x float> undef, <32 x i32> zeroinitializer
  %458 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %457, <32 x float> %446, <32 x float> %442)
  %459 = add nuw nsw i64 %402, 16384
  %460 = getelementptr inbounds [36864 x <32 x float>], [36864 x <32 x float>]* %5, i64 0, i64 0, i64 %459
  %461 = bitcast float* %460 to <32 x float>*
  %462 = load <32 x float>, <32 x float>* %461, align 16, !tbaa !311
  %463 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %417, <32 x float> %462, <32 x float> %447)
  %464 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %423, <32 x float> %462, <32 x float> %448)
  %465 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %429, <32 x float> %462, <32 x float> %449)
  %466 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %435, <32 x float> %462, <32 x float> %450)
  %467 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %441, <32 x float> %462, <32 x float> %451)
  %468 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %457, <32 x float> %462, <32 x float> %452)
  %469 = add nuw nsw i64 %396, 8
  %470 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %469
  %471 = load float, float* %470, align 4, !tbaa !314
  %472 = insertelement <32 x float> undef, float %471, i32 0
  %473 = shufflevector <32 x float> %472, <32 x float> undef, <32 x i32> zeroinitializer
  %474 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %473, <32 x float> %462, <32 x float> %458)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 256
  br i1 %exitcond.1, label %for_end27.1, label %for_body26.1, !prof !34

for_end27.1:                                      ; preds = %for_body26.1
  %475 = add nuw nsw i64 %193, %187
  br label %for_body26.2

for_body26.2:                                     ; preds = %for_body26.2, %for_end27.1
  %indvars.iv.2 = phi i64 [ 0, %for_end27.1 ], [ %indvars.iv.next.2, %for_body26.2 ]
  %476 = phi <32 x float> [ %474, %for_end27.1 ], [ %562, %for_body26.2 ]
  %477 = phi <32 x float> [ %468, %for_end27.1 ], [ %556, %for_body26.2 ]
  %478 = phi <32 x float> [ %467, %for_end27.1 ], [ %555, %for_body26.2 ]
  %479 = phi <32 x float> [ %466, %for_end27.1 ], [ %554, %for_body26.2 ]
  %480 = phi <32 x float> [ %465, %for_end27.1 ], [ %553, %for_body26.2 ]
  %481 = phi <32 x float> [ %464, %for_end27.1 ], [ %552, %for_body26.2 ]
  %482 = phi <32 x float> [ %463, %for_end27.1 ], [ %551, %for_body26.2 ]
  %483 = mul nuw nsw i64 %indvars.iv.2, 30
  %484 = add nuw nsw i64 %475, %483
  %485 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %484
  %486 = load float, float* %485, align 4, !tbaa !314
  %487 = insertelement <32 x float> undef, float %486, i32 0
  %488 = shufflevector <32 x float> %487, <32 x float> undef, <32 x i32> zeroinitializer
  %489 = shl nsw i64 %indvars.iv.2, 5
  %490 = add nuw nsw i64 %188, %489
  %491 = getelementptr inbounds [36864 x <32 x float>], [36864 x <32 x float>]* %5, i64 0, i64 0, i64 %490
  %492 = bitcast float* %491 to <32 x float>*
  %493 = load <32 x float>, <32 x float>* %492, align 16, !tbaa !311
  %494 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %488, <32 x float> %493, <32 x float> %482)
  %495 = add nuw nsw i64 %484, 1
  %496 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %495
  %497 = load float, float* %496, align 4, !tbaa !314
  %498 = insertelement <32 x float> undef, float %497, i32 0
  %499 = shufflevector <32 x float> %498, <32 x float> undef, <32 x i32> zeroinitializer
  %500 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %499, <32 x float> %493, <32 x float> %481)
  %501 = add nuw nsw i64 %484, 2
  %502 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %501
  %503 = load float, float* %502, align 4, !tbaa !314
  %504 = insertelement <32 x float> undef, float %503, i32 0
  %505 = shufflevector <32 x float> %504, <32 x float> undef, <32 x i32> zeroinitializer
  %506 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %505, <32 x float> %493, <32 x float> %480)
  %507 = add nuw nsw i64 %484, 3
  %508 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %507
  %509 = load float, float* %508, align 4, !tbaa !314
  %510 = insertelement <32 x float> undef, float %509, i32 0
  %511 = shufflevector <32 x float> %510, <32 x float> undef, <32 x i32> zeroinitializer
  %512 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %511, <32 x float> %493, <32 x float> %479)
  %513 = add nuw nsw i64 %484, 4
  %514 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %513
  %515 = load float, float* %514, align 4, !tbaa !314
  %516 = insertelement <32 x float> undef, float %515, i32 0
  %517 = shufflevector <32 x float> %516, <32 x float> undef, <32 x i32> zeroinitializer
  %518 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %517, <32 x float> %493, <32 x float> %478)
  %519 = add nuw nsw i64 %484, 5
  %520 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %519
  %521 = load float, float* %520, align 4, !tbaa !314
  %522 = insertelement <32 x float> undef, float %521, i32 0
  %523 = shufflevector <32 x float> %522, <32 x float> undef, <32 x i32> zeroinitializer
  %524 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %523, <32 x float> %493, <32 x float> %477)
  %525 = add nuw nsw i64 %484, 6
  %526 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %525
  %527 = load float, float* %526, align 4, !tbaa !314
  %528 = insertelement <32 x float> undef, float %527, i32 0
  %529 = shufflevector <32 x float> %528, <32 x float> undef, <32 x i32> zeroinitializer
  %530 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %529, <32 x float> %493, <32 x float> %476)
  %531 = add nuw nsw i64 %490, 8192
  %532 = getelementptr inbounds [36864 x <32 x float>], [36864 x <32 x float>]* %5, i64 0, i64 0, i64 %531
  %533 = bitcast float* %532 to <32 x float>*
  %534 = load <32 x float>, <32 x float>* %533, align 16, !tbaa !311
  %535 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %499, <32 x float> %534, <32 x float> %494)
  %536 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %505, <32 x float> %534, <32 x float> %500)
  %537 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %511, <32 x float> %534, <32 x float> %506)
  %538 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %517, <32 x float> %534, <32 x float> %512)
  %539 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %523, <32 x float> %534, <32 x float> %518)
  %540 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %529, <32 x float> %534, <32 x float> %524)
  %541 = add nuw nsw i64 %484, 7
  %542 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %541
  %543 = load float, float* %542, align 4, !tbaa !314
  %544 = insertelement <32 x float> undef, float %543, i32 0
  %545 = shufflevector <32 x float> %544, <32 x float> undef, <32 x i32> zeroinitializer
  %546 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %545, <32 x float> %534, <32 x float> %530)
  %547 = add nuw nsw i64 %490, 16384
  %548 = getelementptr inbounds [36864 x <32 x float>], [36864 x <32 x float>]* %5, i64 0, i64 0, i64 %547
  %549 = bitcast float* %548 to <32 x float>*
  %550 = load <32 x float>, <32 x float>* %549, align 16, !tbaa !311
  %551 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %505, <32 x float> %550, <32 x float> %535)
  %552 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %511, <32 x float> %550, <32 x float> %536)
  %553 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %517, <32 x float> %550, <32 x float> %537)
  %554 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %523, <32 x float> %550, <32 x float> %538)
  %555 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %529, <32 x float> %550, <32 x float> %539)
  %556 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %545, <32 x float> %550, <32 x float> %540)
  %557 = add nuw nsw i64 %484, 8
  %558 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %557
  %559 = load float, float* %558, align 4, !tbaa !314
  %560 = insertelement <32 x float> undef, float %559, i32 0
  %561 = shufflevector <32 x float> %560, <32 x float> undef, <32 x i32> zeroinitializer
  %562 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %561, <32 x float> %550, <32 x float> %546)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 256
  br i1 %exitcond.2, label %for_end27.2, label %for_body26.2, !prof !34

for_end27.2:                                      ; preds = %for_body26.2
  store <32 x float> %551, <32 x float>* %.sub, align 128, !tbaa !323
  store <32 x float> %552, <32 x float>* %25, align 128, !tbaa !323
  store <32 x float> %553, <32 x float>* %27, align 128, !tbaa !323
  store <32 x float> %554, <32 x float>* %29, align 128, !tbaa !323
  store <32 x float> %555, <32 x float>* %31, align 128, !tbaa !323
  store <32 x float> %556, <32 x float>* %33, align 128, !tbaa !323
  store <32 x float> %562, <32 x float>* %35, align 128, !tbaa !323
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep48, i8* nonnull align 128 %14, i64 896, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond49 = icmp eq i64 %indvar.next, 4
  br i1 %exitcond49, label %for_begin31.preheader, label %for_body20, !prof !34

if_end.us.us.29:                                  ; preds = %for_begin1.preheader, %if_end.us.us.29
  %indvars.iv81 = phi i64 [ %indvars.iv.next82, %if_end.us.us.29 ], [ 0, %for_begin1.preheader ]
  %563 = mul nuw nsw i64 %indvars.iv81, 30
  %564 = add nuw nsw i64 %563, %8
  %565 = mul nuw nsw i64 %indvars.iv81, 784
  %566 = add nsw i64 %13, %565
  %567 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %564
  store float 0.000000e+00, float* %567, align 8, !tbaa !314
  %568 = or i64 %564, 1
  %569 = add nsw i64 %566, 1
  %570 = getelementptr inbounds float, float* %7, i64 %569
  %571 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %568
  %572 = bitcast float* %570 to <4 x i32>*
  %573 = load <4 x i32>, <4 x i32>* %572, align 4, !tbaa !332
  %574 = bitcast float* %571 to <4 x i32>*
  store <4 x i32> %573, <4 x i32>* %574, align 4, !tbaa !314
  %575 = add nuw nsw i64 %564, 5
  %576 = add nsw i64 %566, 5
  %577 = getelementptr inbounds float, float* %7, i64 %576
  %578 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %575
  %579 = bitcast float* %577 to <4 x i32>*
  %580 = load <4 x i32>, <4 x i32>* %579, align 4, !tbaa !332
  %581 = bitcast float* %578 to <4 x i32>*
  store <4 x i32> %580, <4 x i32>* %581, align 4, !tbaa !314
  %582 = add nuw nsw i64 %564, 9
  %583 = add nsw i64 %566, 9
  %584 = getelementptr inbounds float, float* %7, i64 %583
  %585 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %582
  %586 = bitcast float* %584 to <4 x i32>*
  %587 = load <4 x i32>, <4 x i32>* %586, align 4, !tbaa !332
  %588 = bitcast float* %585 to <4 x i32>*
  store <4 x i32> %587, <4 x i32>* %588, align 4, !tbaa !314
  %589 = add nuw nsw i64 %564, 13
  %590 = add nsw i64 %566, 13
  %591 = getelementptr inbounds float, float* %7, i64 %590
  %592 = bitcast float* %591 to i32*
  %593 = load i32, i32* %592, align 4, !tbaa !332
  %594 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %589
  %595 = bitcast float* %594 to i32*
  store i32 %593, i32* %595, align 4, !tbaa !314
  %596 = add nuw nsw i64 %564, 14
  %597 = add nsw i64 %566, 14
  %598 = getelementptr inbounds float, float* %7, i64 %597
  %599 = bitcast float* %598 to i32*
  %600 = load i32, i32* %599, align 4, !tbaa !332
  %601 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %596
  %602 = bitcast float* %601 to i32*
  store i32 %600, i32* %602, align 8, !tbaa !314
  %603 = add nuw nsw i64 %564, 15
  %604 = add nsw i64 %566, 15
  %605 = getelementptr inbounds float, float* %7, i64 %604
  %606 = bitcast float* %605 to i32*
  %607 = load i32, i32* %606, align 4, !tbaa !332
  %608 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %603
  %609 = bitcast float* %608 to i32*
  store i32 %607, i32* %609, align 4, !tbaa !314
  %610 = add nuw nsw i64 %564, 16
  %611 = add nsw i64 %566, 16
  %612 = getelementptr inbounds float, float* %7, i64 %611
  %613 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %610
  %614 = bitcast float* %612 to <4 x i32>*
  %615 = load <4 x i32>, <4 x i32>* %614, align 4, !tbaa !332
  %616 = bitcast float* %613 to <4 x i32>*
  store <4 x i32> %615, <4 x i32>* %616, align 8, !tbaa !314
  %617 = add nuw nsw i64 %564, 20
  %618 = add nsw i64 %566, 20
  %619 = getelementptr inbounds float, float* %7, i64 %618
  %620 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %617
  %621 = bitcast float* %619 to <4 x i32>*
  %622 = load <4 x i32>, <4 x i32>* %621, align 4, !tbaa !332
  %623 = bitcast float* %620 to <4 x i32>*
  store <4 x i32> %622, <4 x i32>* %623, align 8, !tbaa !314
  %624 = add nuw nsw i64 %564, 24
  %625 = add nsw i64 %566, 24
  %626 = getelementptr inbounds float, float* %7, i64 %625
  %627 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %624
  %628 = bitcast float* %626 to <4 x i32>*
  %629 = load <4 x i32>, <4 x i32>* %628, align 4, !tbaa !332
  %630 = bitcast float* %627 to <4 x i32>*
  store <4 x i32> %629, <4 x i32>* %630, align 8, !tbaa !314
  %631 = add nuw nsw i64 %564, 28
  %632 = add nsw i64 %566, 28
  %633 = getelementptr inbounds float, float* %7, i64 %632
  %634 = bitcast float* %633 to i32*
  %635 = load i32, i32* %634, align 4, !tbaa !332
  %636 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %631
  %637 = bitcast float* %636 to i32*
  store i32 %635, i32* %637, align 8, !tbaa !314
  %638 = add nuw nsw i64 %564, 29
  %639 = getelementptr inbounds [230400 x float], [230400 x float]* %6, i64 0, i64 %638
  store float 0.000000e+00, float* %639, align 4, !tbaa !314
  %indvars.iv.next82 = add nuw nsw i64 %indvars.iv81, 1
  %exitcond83 = icmp eq i64 %indvars.iv.next82, 256
  br i1 %exitcond83, label %for_end3, label %if_end.us.us.29, !prof !34
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_relu_5(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_relu_5_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_relu_5_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %1 to float*
  %3 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv4 = phi i64 [ 0, %entry ], [ %indvars.iv.next5, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv4, 50176
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv1 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next2, %for_begin4.preheader ]
  %5 = mul nuw nsw i64 %indvars.iv1, 224
  %6 = add nuw nsw i64 %5, %4
  %7 = getelementptr inbounds float, float* %2, i64 %6
  %8 = bitcast float* %7 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !335
  %9 = getelementptr inbounds float, float* %7, i64 4
  %10 = bitcast float* %9 to <4 x float>*
  %wide.load8 = load <4 x float>, <4 x float>* %10, align 4, !tbaa !335
  %11 = fcmp ogt <4 x float> %wide.load, zeroinitializer
  %12 = fcmp ogt <4 x float> %wide.load8, zeroinitializer
  %13 = select <4 x i1> %11, <4 x float> %wide.load, <4 x float> zeroinitializer
  %14 = select <4 x i1> %12, <4 x float> %wide.load8, <4 x float> zeroinitializer
  %15 = getelementptr inbounds float, float* %3, i64 %6
  %16 = bitcast float* %15 to <4 x float>*
  store <4 x float> %13, <4 x float>* %16, align 4, !tbaa !338
  %17 = getelementptr inbounds float, float* %15, i64 4
  %18 = bitcast float* %17 to <4 x float>*
  store <4 x float> %14, <4 x float>* %18, align 4, !tbaa !338
  %19 = or i64 %6, 8
  %20 = getelementptr inbounds float, float* %2, i64 %19
  %21 = bitcast float* %20 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %21, align 4, !tbaa !335
  %22 = getelementptr inbounds float, float* %20, i64 4
  %23 = bitcast float* %22 to <4 x float>*
  %wide.load8.1 = load <4 x float>, <4 x float>* %23, align 4, !tbaa !335
  %24 = fcmp ogt <4 x float> %wide.load.1, zeroinitializer
  %25 = fcmp ogt <4 x float> %wide.load8.1, zeroinitializer
  %26 = select <4 x i1> %24, <4 x float> %wide.load.1, <4 x float> zeroinitializer
  %27 = select <4 x i1> %25, <4 x float> %wide.load8.1, <4 x float> zeroinitializer
  %28 = getelementptr inbounds float, float* %3, i64 %19
  %29 = bitcast float* %28 to <4 x float>*
  store <4 x float> %26, <4 x float>* %29, align 4, !tbaa !338
  %30 = getelementptr inbounds float, float* %28, i64 4
  %31 = bitcast float* %30 to <4 x float>*
  store <4 x float> %27, <4 x float>* %31, align 4, !tbaa !338
  %32 = or i64 %6, 16
  %33 = getelementptr inbounds float, float* %2, i64 %32
  %34 = bitcast float* %33 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %34, align 4, !tbaa !335
  %35 = getelementptr inbounds float, float* %33, i64 4
  %36 = bitcast float* %35 to <4 x float>*
  %wide.load8.2 = load <4 x float>, <4 x float>* %36, align 4, !tbaa !335
  %37 = fcmp ogt <4 x float> %wide.load.2, zeroinitializer
  %38 = fcmp ogt <4 x float> %wide.load8.2, zeroinitializer
  %39 = select <4 x i1> %37, <4 x float> %wide.load.2, <4 x float> zeroinitializer
  %40 = select <4 x i1> %38, <4 x float> %wide.load8.2, <4 x float> zeroinitializer
  %41 = getelementptr inbounds float, float* %3, i64 %32
  %42 = bitcast float* %41 to <4 x float>*
  store <4 x float> %39, <4 x float>* %42, align 4, !tbaa !338
  %43 = getelementptr inbounds float, float* %41, i64 4
  %44 = bitcast float* %43 to <4 x float>*
  store <4 x float> %40, <4 x float>* %44, align 4, !tbaa !338
  %45 = or i64 %6, 24
  %46 = getelementptr inbounds float, float* %2, i64 %45
  %47 = bitcast float* %46 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %47, align 4, !tbaa !335
  %48 = getelementptr inbounds float, float* %46, i64 4
  %49 = bitcast float* %48 to <4 x float>*
  %wide.load8.3 = load <4 x float>, <4 x float>* %49, align 4, !tbaa !335
  %50 = fcmp ogt <4 x float> %wide.load.3, zeroinitializer
  %51 = fcmp ogt <4 x float> %wide.load8.3, zeroinitializer
  %52 = select <4 x i1> %50, <4 x float> %wide.load.3, <4 x float> zeroinitializer
  %53 = select <4 x i1> %51, <4 x float> %wide.load8.3, <4 x float> zeroinitializer
  %54 = getelementptr inbounds float, float* %3, i64 %45
  %55 = bitcast float* %54 to <4 x float>*
  store <4 x float> %52, <4 x float>* %55, align 4, !tbaa !338
  %56 = getelementptr inbounds float, float* %54, i64 4
  %57 = bitcast float* %56 to <4 x float>*
  store <4 x float> %53, <4 x float>* %57, align 4, !tbaa !338
  %58 = add nuw nsw i64 %6, 32
  %59 = getelementptr inbounds float, float* %2, i64 %58
  %60 = bitcast float* %59 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %60, align 4, !tbaa !335
  %61 = getelementptr inbounds float, float* %59, i64 4
  %62 = bitcast float* %61 to <4 x float>*
  %wide.load8.4 = load <4 x float>, <4 x float>* %62, align 4, !tbaa !335
  %63 = fcmp ogt <4 x float> %wide.load.4, zeroinitializer
  %64 = fcmp ogt <4 x float> %wide.load8.4, zeroinitializer
  %65 = select <4 x i1> %63, <4 x float> %wide.load.4, <4 x float> zeroinitializer
  %66 = select <4 x i1> %64, <4 x float> %wide.load8.4, <4 x float> zeroinitializer
  %67 = getelementptr inbounds float, float* %3, i64 %58
  %68 = bitcast float* %67 to <4 x float>*
  store <4 x float> %65, <4 x float>* %68, align 4, !tbaa !338
  %69 = getelementptr inbounds float, float* %67, i64 4
  %70 = bitcast float* %69 to <4 x float>*
  store <4 x float> %66, <4 x float>* %70, align 4, !tbaa !338
  %71 = add nuw nsw i64 %6, 40
  %72 = getelementptr inbounds float, float* %2, i64 %71
  %73 = bitcast float* %72 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %73, align 4, !tbaa !335
  %74 = getelementptr inbounds float, float* %72, i64 4
  %75 = bitcast float* %74 to <4 x float>*
  %wide.load8.5 = load <4 x float>, <4 x float>* %75, align 4, !tbaa !335
  %76 = fcmp ogt <4 x float> %wide.load.5, zeroinitializer
  %77 = fcmp ogt <4 x float> %wide.load8.5, zeroinitializer
  %78 = select <4 x i1> %76, <4 x float> %wide.load.5, <4 x float> zeroinitializer
  %79 = select <4 x i1> %77, <4 x float> %wide.load8.5, <4 x float> zeroinitializer
  %80 = getelementptr inbounds float, float* %3, i64 %71
  %81 = bitcast float* %80 to <4 x float>*
  store <4 x float> %78, <4 x float>* %81, align 4, !tbaa !338
  %82 = getelementptr inbounds float, float* %80, i64 4
  %83 = bitcast float* %82 to <4 x float>*
  store <4 x float> %79, <4 x float>* %83, align 4, !tbaa !338
  %84 = add nuw nsw i64 %6, 48
  %85 = getelementptr inbounds float, float* %2, i64 %84
  %86 = bitcast float* %85 to <4 x float>*
  %wide.load.6 = load <4 x float>, <4 x float>* %86, align 4, !tbaa !335
  %87 = getelementptr inbounds float, float* %85, i64 4
  %88 = bitcast float* %87 to <4 x float>*
  %wide.load8.6 = load <4 x float>, <4 x float>* %88, align 4, !tbaa !335
  %89 = fcmp ogt <4 x float> %wide.load.6, zeroinitializer
  %90 = fcmp ogt <4 x float> %wide.load8.6, zeroinitializer
  %91 = select <4 x i1> %89, <4 x float> %wide.load.6, <4 x float> zeroinitializer
  %92 = select <4 x i1> %90, <4 x float> %wide.load8.6, <4 x float> zeroinitializer
  %93 = getelementptr inbounds float, float* %3, i64 %84
  %94 = bitcast float* %93 to <4 x float>*
  store <4 x float> %91, <4 x float>* %94, align 4, !tbaa !338
  %95 = getelementptr inbounds float, float* %93, i64 4
  %96 = bitcast float* %95 to <4 x float>*
  store <4 x float> %92, <4 x float>* %96, align 4, !tbaa !338
  %97 = add nuw nsw i64 %6, 56
  %98 = getelementptr inbounds float, float* %2, i64 %97
  %99 = bitcast float* %98 to <4 x float>*
  %wide.load.7 = load <4 x float>, <4 x float>* %99, align 4, !tbaa !335
  %100 = getelementptr inbounds float, float* %98, i64 4
  %101 = bitcast float* %100 to <4 x float>*
  %wide.load8.7 = load <4 x float>, <4 x float>* %101, align 4, !tbaa !335
  %102 = fcmp ogt <4 x float> %wide.load.7, zeroinitializer
  %103 = fcmp ogt <4 x float> %wide.load8.7, zeroinitializer
  %104 = select <4 x i1> %102, <4 x float> %wide.load.7, <4 x float> zeroinitializer
  %105 = select <4 x i1> %103, <4 x float> %wide.load8.7, <4 x float> zeroinitializer
  %106 = getelementptr inbounds float, float* %3, i64 %97
  %107 = bitcast float* %106 to <4 x float>*
  store <4 x float> %104, <4 x float>* %107, align 4, !tbaa !338
  %108 = getelementptr inbounds float, float* %106, i64 4
  %109 = bitcast float* %108 to <4 x float>*
  store <4 x float> %105, <4 x float>* %109, align 4, !tbaa !338
  %110 = add nuw nsw i64 %6, 64
  %111 = getelementptr inbounds float, float* %2, i64 %110
  %112 = bitcast float* %111 to <4 x float>*
  %wide.load.8 = load <4 x float>, <4 x float>* %112, align 4, !tbaa !335
  %113 = getelementptr inbounds float, float* %111, i64 4
  %114 = bitcast float* %113 to <4 x float>*
  %wide.load8.8 = load <4 x float>, <4 x float>* %114, align 4, !tbaa !335
  %115 = fcmp ogt <4 x float> %wide.load.8, zeroinitializer
  %116 = fcmp ogt <4 x float> %wide.load8.8, zeroinitializer
  %117 = select <4 x i1> %115, <4 x float> %wide.load.8, <4 x float> zeroinitializer
  %118 = select <4 x i1> %116, <4 x float> %wide.load8.8, <4 x float> zeroinitializer
  %119 = getelementptr inbounds float, float* %3, i64 %110
  %120 = bitcast float* %119 to <4 x float>*
  store <4 x float> %117, <4 x float>* %120, align 4, !tbaa !338
  %121 = getelementptr inbounds float, float* %119, i64 4
  %122 = bitcast float* %121 to <4 x float>*
  store <4 x float> %118, <4 x float>* %122, align 4, !tbaa !338
  %123 = add nuw nsw i64 %6, 72
  %124 = getelementptr inbounds float, float* %2, i64 %123
  %125 = bitcast float* %124 to <4 x float>*
  %wide.load.9 = load <4 x float>, <4 x float>* %125, align 4, !tbaa !335
  %126 = getelementptr inbounds float, float* %124, i64 4
  %127 = bitcast float* %126 to <4 x float>*
  %wide.load8.9 = load <4 x float>, <4 x float>* %127, align 4, !tbaa !335
  %128 = fcmp ogt <4 x float> %wide.load.9, zeroinitializer
  %129 = fcmp ogt <4 x float> %wide.load8.9, zeroinitializer
  %130 = select <4 x i1> %128, <4 x float> %wide.load.9, <4 x float> zeroinitializer
  %131 = select <4 x i1> %129, <4 x float> %wide.load8.9, <4 x float> zeroinitializer
  %132 = getelementptr inbounds float, float* %3, i64 %123
  %133 = bitcast float* %132 to <4 x float>*
  store <4 x float> %130, <4 x float>* %133, align 4, !tbaa !338
  %134 = getelementptr inbounds float, float* %132, i64 4
  %135 = bitcast float* %134 to <4 x float>*
  store <4 x float> %131, <4 x float>* %135, align 4, !tbaa !338
  %136 = add nuw nsw i64 %6, 80
  %137 = getelementptr inbounds float, float* %2, i64 %136
  %138 = bitcast float* %137 to <4 x float>*
  %wide.load.10 = load <4 x float>, <4 x float>* %138, align 4, !tbaa !335
  %139 = getelementptr inbounds float, float* %137, i64 4
  %140 = bitcast float* %139 to <4 x float>*
  %wide.load8.10 = load <4 x float>, <4 x float>* %140, align 4, !tbaa !335
  %141 = fcmp ogt <4 x float> %wide.load.10, zeroinitializer
  %142 = fcmp ogt <4 x float> %wide.load8.10, zeroinitializer
  %143 = select <4 x i1> %141, <4 x float> %wide.load.10, <4 x float> zeroinitializer
  %144 = select <4 x i1> %142, <4 x float> %wide.load8.10, <4 x float> zeroinitializer
  %145 = getelementptr inbounds float, float* %3, i64 %136
  %146 = bitcast float* %145 to <4 x float>*
  store <4 x float> %143, <4 x float>* %146, align 4, !tbaa !338
  %147 = getelementptr inbounds float, float* %145, i64 4
  %148 = bitcast float* %147 to <4 x float>*
  store <4 x float> %144, <4 x float>* %148, align 4, !tbaa !338
  %149 = add nuw nsw i64 %6, 88
  %150 = getelementptr inbounds float, float* %2, i64 %149
  %151 = bitcast float* %150 to <4 x float>*
  %wide.load.11 = load <4 x float>, <4 x float>* %151, align 4, !tbaa !335
  %152 = getelementptr inbounds float, float* %150, i64 4
  %153 = bitcast float* %152 to <4 x float>*
  %wide.load8.11 = load <4 x float>, <4 x float>* %153, align 4, !tbaa !335
  %154 = fcmp ogt <4 x float> %wide.load.11, zeroinitializer
  %155 = fcmp ogt <4 x float> %wide.load8.11, zeroinitializer
  %156 = select <4 x i1> %154, <4 x float> %wide.load.11, <4 x float> zeroinitializer
  %157 = select <4 x i1> %155, <4 x float> %wide.load8.11, <4 x float> zeroinitializer
  %158 = getelementptr inbounds float, float* %3, i64 %149
  %159 = bitcast float* %158 to <4 x float>*
  store <4 x float> %156, <4 x float>* %159, align 4, !tbaa !338
  %160 = getelementptr inbounds float, float* %158, i64 4
  %161 = bitcast float* %160 to <4 x float>*
  store <4 x float> %157, <4 x float>* %161, align 4, !tbaa !338
  %162 = add nuw nsw i64 %6, 96
  %163 = getelementptr inbounds float, float* %2, i64 %162
  %164 = bitcast float* %163 to <4 x float>*
  %wide.load.12 = load <4 x float>, <4 x float>* %164, align 4, !tbaa !335
  %165 = getelementptr inbounds float, float* %163, i64 4
  %166 = bitcast float* %165 to <4 x float>*
  %wide.load8.12 = load <4 x float>, <4 x float>* %166, align 4, !tbaa !335
  %167 = fcmp ogt <4 x float> %wide.load.12, zeroinitializer
  %168 = fcmp ogt <4 x float> %wide.load8.12, zeroinitializer
  %169 = select <4 x i1> %167, <4 x float> %wide.load.12, <4 x float> zeroinitializer
  %170 = select <4 x i1> %168, <4 x float> %wide.load8.12, <4 x float> zeroinitializer
  %171 = getelementptr inbounds float, float* %3, i64 %162
  %172 = bitcast float* %171 to <4 x float>*
  store <4 x float> %169, <4 x float>* %172, align 4, !tbaa !338
  %173 = getelementptr inbounds float, float* %171, i64 4
  %174 = bitcast float* %173 to <4 x float>*
  store <4 x float> %170, <4 x float>* %174, align 4, !tbaa !338
  %175 = add nuw nsw i64 %6, 104
  %176 = getelementptr inbounds float, float* %2, i64 %175
  %177 = bitcast float* %176 to <4 x float>*
  %wide.load.13 = load <4 x float>, <4 x float>* %177, align 4, !tbaa !335
  %178 = getelementptr inbounds float, float* %176, i64 4
  %179 = bitcast float* %178 to <4 x float>*
  %wide.load8.13 = load <4 x float>, <4 x float>* %179, align 4, !tbaa !335
  %180 = fcmp ogt <4 x float> %wide.load.13, zeroinitializer
  %181 = fcmp ogt <4 x float> %wide.load8.13, zeroinitializer
  %182 = select <4 x i1> %180, <4 x float> %wide.load.13, <4 x float> zeroinitializer
  %183 = select <4 x i1> %181, <4 x float> %wide.load8.13, <4 x float> zeroinitializer
  %184 = getelementptr inbounds float, float* %3, i64 %175
  %185 = bitcast float* %184 to <4 x float>*
  store <4 x float> %182, <4 x float>* %185, align 4, !tbaa !338
  %186 = getelementptr inbounds float, float* %184, i64 4
  %187 = bitcast float* %186 to <4 x float>*
  store <4 x float> %183, <4 x float>* %187, align 4, !tbaa !338
  %188 = add nuw nsw i64 %6, 112
  %189 = getelementptr inbounds float, float* %2, i64 %188
  %190 = bitcast float* %189 to <4 x float>*
  %wide.load.14 = load <4 x float>, <4 x float>* %190, align 4, !tbaa !335
  %191 = getelementptr inbounds float, float* %189, i64 4
  %192 = bitcast float* %191 to <4 x float>*
  %wide.load8.14 = load <4 x float>, <4 x float>* %192, align 4, !tbaa !335
  %193 = fcmp ogt <4 x float> %wide.load.14, zeroinitializer
  %194 = fcmp ogt <4 x float> %wide.load8.14, zeroinitializer
  %195 = select <4 x i1> %193, <4 x float> %wide.load.14, <4 x float> zeroinitializer
  %196 = select <4 x i1> %194, <4 x float> %wide.load8.14, <4 x float> zeroinitializer
  %197 = getelementptr inbounds float, float* %3, i64 %188
  %198 = bitcast float* %197 to <4 x float>*
  store <4 x float> %195, <4 x float>* %198, align 4, !tbaa !338
  %199 = getelementptr inbounds float, float* %197, i64 4
  %200 = bitcast float* %199 to <4 x float>*
  store <4 x float> %196, <4 x float>* %200, align 4, !tbaa !338
  %201 = add nuw nsw i64 %6, 120
  %202 = getelementptr inbounds float, float* %2, i64 %201
  %203 = bitcast float* %202 to <4 x float>*
  %wide.load.15 = load <4 x float>, <4 x float>* %203, align 4, !tbaa !335
  %204 = getelementptr inbounds float, float* %202, i64 4
  %205 = bitcast float* %204 to <4 x float>*
  %wide.load8.15 = load <4 x float>, <4 x float>* %205, align 4, !tbaa !335
  %206 = fcmp ogt <4 x float> %wide.load.15, zeroinitializer
  %207 = fcmp ogt <4 x float> %wide.load8.15, zeroinitializer
  %208 = select <4 x i1> %206, <4 x float> %wide.load.15, <4 x float> zeroinitializer
  %209 = select <4 x i1> %207, <4 x float> %wide.load8.15, <4 x float> zeroinitializer
  %210 = getelementptr inbounds float, float* %3, i64 %201
  %211 = bitcast float* %210 to <4 x float>*
  store <4 x float> %208, <4 x float>* %211, align 4, !tbaa !338
  %212 = getelementptr inbounds float, float* %210, i64 4
  %213 = bitcast float* %212 to <4 x float>*
  store <4 x float> %209, <4 x float>* %213, align 4, !tbaa !338
  %214 = add nuw nsw i64 %6, 128
  %215 = getelementptr inbounds float, float* %2, i64 %214
  %216 = bitcast float* %215 to <4 x float>*
  %wide.load.16 = load <4 x float>, <4 x float>* %216, align 4, !tbaa !335
  %217 = getelementptr inbounds float, float* %215, i64 4
  %218 = bitcast float* %217 to <4 x float>*
  %wide.load8.16 = load <4 x float>, <4 x float>* %218, align 4, !tbaa !335
  %219 = fcmp ogt <4 x float> %wide.load.16, zeroinitializer
  %220 = fcmp ogt <4 x float> %wide.load8.16, zeroinitializer
  %221 = select <4 x i1> %219, <4 x float> %wide.load.16, <4 x float> zeroinitializer
  %222 = select <4 x i1> %220, <4 x float> %wide.load8.16, <4 x float> zeroinitializer
  %223 = getelementptr inbounds float, float* %3, i64 %214
  %224 = bitcast float* %223 to <4 x float>*
  store <4 x float> %221, <4 x float>* %224, align 4, !tbaa !338
  %225 = getelementptr inbounds float, float* %223, i64 4
  %226 = bitcast float* %225 to <4 x float>*
  store <4 x float> %222, <4 x float>* %226, align 4, !tbaa !338
  %227 = add nuw nsw i64 %6, 136
  %228 = getelementptr inbounds float, float* %2, i64 %227
  %229 = bitcast float* %228 to <4 x float>*
  %wide.load.17 = load <4 x float>, <4 x float>* %229, align 4, !tbaa !335
  %230 = getelementptr inbounds float, float* %228, i64 4
  %231 = bitcast float* %230 to <4 x float>*
  %wide.load8.17 = load <4 x float>, <4 x float>* %231, align 4, !tbaa !335
  %232 = fcmp ogt <4 x float> %wide.load.17, zeroinitializer
  %233 = fcmp ogt <4 x float> %wide.load8.17, zeroinitializer
  %234 = select <4 x i1> %232, <4 x float> %wide.load.17, <4 x float> zeroinitializer
  %235 = select <4 x i1> %233, <4 x float> %wide.load8.17, <4 x float> zeroinitializer
  %236 = getelementptr inbounds float, float* %3, i64 %227
  %237 = bitcast float* %236 to <4 x float>*
  store <4 x float> %234, <4 x float>* %237, align 4, !tbaa !338
  %238 = getelementptr inbounds float, float* %236, i64 4
  %239 = bitcast float* %238 to <4 x float>*
  store <4 x float> %235, <4 x float>* %239, align 4, !tbaa !338
  %240 = add nuw nsw i64 %6, 144
  %241 = getelementptr inbounds float, float* %2, i64 %240
  %242 = bitcast float* %241 to <4 x float>*
  %wide.load.18 = load <4 x float>, <4 x float>* %242, align 4, !tbaa !335
  %243 = getelementptr inbounds float, float* %241, i64 4
  %244 = bitcast float* %243 to <4 x float>*
  %wide.load8.18 = load <4 x float>, <4 x float>* %244, align 4, !tbaa !335
  %245 = fcmp ogt <4 x float> %wide.load.18, zeroinitializer
  %246 = fcmp ogt <4 x float> %wide.load8.18, zeroinitializer
  %247 = select <4 x i1> %245, <4 x float> %wide.load.18, <4 x float> zeroinitializer
  %248 = select <4 x i1> %246, <4 x float> %wide.load8.18, <4 x float> zeroinitializer
  %249 = getelementptr inbounds float, float* %3, i64 %240
  %250 = bitcast float* %249 to <4 x float>*
  store <4 x float> %247, <4 x float>* %250, align 4, !tbaa !338
  %251 = getelementptr inbounds float, float* %249, i64 4
  %252 = bitcast float* %251 to <4 x float>*
  store <4 x float> %248, <4 x float>* %252, align 4, !tbaa !338
  %253 = add nuw nsw i64 %6, 152
  %254 = getelementptr inbounds float, float* %2, i64 %253
  %255 = bitcast float* %254 to <4 x float>*
  %wide.load.19 = load <4 x float>, <4 x float>* %255, align 4, !tbaa !335
  %256 = getelementptr inbounds float, float* %254, i64 4
  %257 = bitcast float* %256 to <4 x float>*
  %wide.load8.19 = load <4 x float>, <4 x float>* %257, align 4, !tbaa !335
  %258 = fcmp ogt <4 x float> %wide.load.19, zeroinitializer
  %259 = fcmp ogt <4 x float> %wide.load8.19, zeroinitializer
  %260 = select <4 x i1> %258, <4 x float> %wide.load.19, <4 x float> zeroinitializer
  %261 = select <4 x i1> %259, <4 x float> %wide.load8.19, <4 x float> zeroinitializer
  %262 = getelementptr inbounds float, float* %3, i64 %253
  %263 = bitcast float* %262 to <4 x float>*
  store <4 x float> %260, <4 x float>* %263, align 4, !tbaa !338
  %264 = getelementptr inbounds float, float* %262, i64 4
  %265 = bitcast float* %264 to <4 x float>*
  store <4 x float> %261, <4 x float>* %265, align 4, !tbaa !338
  %266 = add nuw nsw i64 %6, 160
  %267 = getelementptr inbounds float, float* %2, i64 %266
  %268 = bitcast float* %267 to <4 x float>*
  %wide.load.20 = load <4 x float>, <4 x float>* %268, align 4, !tbaa !335
  %269 = getelementptr inbounds float, float* %267, i64 4
  %270 = bitcast float* %269 to <4 x float>*
  %wide.load8.20 = load <4 x float>, <4 x float>* %270, align 4, !tbaa !335
  %271 = fcmp ogt <4 x float> %wide.load.20, zeroinitializer
  %272 = fcmp ogt <4 x float> %wide.load8.20, zeroinitializer
  %273 = select <4 x i1> %271, <4 x float> %wide.load.20, <4 x float> zeroinitializer
  %274 = select <4 x i1> %272, <4 x float> %wide.load8.20, <4 x float> zeroinitializer
  %275 = getelementptr inbounds float, float* %3, i64 %266
  %276 = bitcast float* %275 to <4 x float>*
  store <4 x float> %273, <4 x float>* %276, align 4, !tbaa !338
  %277 = getelementptr inbounds float, float* %275, i64 4
  %278 = bitcast float* %277 to <4 x float>*
  store <4 x float> %274, <4 x float>* %278, align 4, !tbaa !338
  %279 = add nuw nsw i64 %6, 168
  %280 = getelementptr inbounds float, float* %2, i64 %279
  %281 = bitcast float* %280 to <4 x float>*
  %wide.load.21 = load <4 x float>, <4 x float>* %281, align 4, !tbaa !335
  %282 = getelementptr inbounds float, float* %280, i64 4
  %283 = bitcast float* %282 to <4 x float>*
  %wide.load8.21 = load <4 x float>, <4 x float>* %283, align 4, !tbaa !335
  %284 = fcmp ogt <4 x float> %wide.load.21, zeroinitializer
  %285 = fcmp ogt <4 x float> %wide.load8.21, zeroinitializer
  %286 = select <4 x i1> %284, <4 x float> %wide.load.21, <4 x float> zeroinitializer
  %287 = select <4 x i1> %285, <4 x float> %wide.load8.21, <4 x float> zeroinitializer
  %288 = getelementptr inbounds float, float* %3, i64 %279
  %289 = bitcast float* %288 to <4 x float>*
  store <4 x float> %286, <4 x float>* %289, align 4, !tbaa !338
  %290 = getelementptr inbounds float, float* %288, i64 4
  %291 = bitcast float* %290 to <4 x float>*
  store <4 x float> %287, <4 x float>* %291, align 4, !tbaa !338
  %292 = add nuw nsw i64 %6, 176
  %293 = getelementptr inbounds float, float* %2, i64 %292
  %294 = bitcast float* %293 to <4 x float>*
  %wide.load.22 = load <4 x float>, <4 x float>* %294, align 4, !tbaa !335
  %295 = getelementptr inbounds float, float* %293, i64 4
  %296 = bitcast float* %295 to <4 x float>*
  %wide.load8.22 = load <4 x float>, <4 x float>* %296, align 4, !tbaa !335
  %297 = fcmp ogt <4 x float> %wide.load.22, zeroinitializer
  %298 = fcmp ogt <4 x float> %wide.load8.22, zeroinitializer
  %299 = select <4 x i1> %297, <4 x float> %wide.load.22, <4 x float> zeroinitializer
  %300 = select <4 x i1> %298, <4 x float> %wide.load8.22, <4 x float> zeroinitializer
  %301 = getelementptr inbounds float, float* %3, i64 %292
  %302 = bitcast float* %301 to <4 x float>*
  store <4 x float> %299, <4 x float>* %302, align 4, !tbaa !338
  %303 = getelementptr inbounds float, float* %301, i64 4
  %304 = bitcast float* %303 to <4 x float>*
  store <4 x float> %300, <4 x float>* %304, align 4, !tbaa !338
  %305 = add nuw nsw i64 %6, 184
  %306 = getelementptr inbounds float, float* %2, i64 %305
  %307 = bitcast float* %306 to <4 x float>*
  %wide.load.23 = load <4 x float>, <4 x float>* %307, align 4, !tbaa !335
  %308 = getelementptr inbounds float, float* %306, i64 4
  %309 = bitcast float* %308 to <4 x float>*
  %wide.load8.23 = load <4 x float>, <4 x float>* %309, align 4, !tbaa !335
  %310 = fcmp ogt <4 x float> %wide.load.23, zeroinitializer
  %311 = fcmp ogt <4 x float> %wide.load8.23, zeroinitializer
  %312 = select <4 x i1> %310, <4 x float> %wide.load.23, <4 x float> zeroinitializer
  %313 = select <4 x i1> %311, <4 x float> %wide.load8.23, <4 x float> zeroinitializer
  %314 = getelementptr inbounds float, float* %3, i64 %305
  %315 = bitcast float* %314 to <4 x float>*
  store <4 x float> %312, <4 x float>* %315, align 4, !tbaa !338
  %316 = getelementptr inbounds float, float* %314, i64 4
  %317 = bitcast float* %316 to <4 x float>*
  store <4 x float> %313, <4 x float>* %317, align 4, !tbaa !338
  %318 = add nuw nsw i64 %6, 192
  %319 = getelementptr inbounds float, float* %2, i64 %318
  %320 = bitcast float* %319 to <4 x float>*
  %wide.load.24 = load <4 x float>, <4 x float>* %320, align 4, !tbaa !335
  %321 = getelementptr inbounds float, float* %319, i64 4
  %322 = bitcast float* %321 to <4 x float>*
  %wide.load8.24 = load <4 x float>, <4 x float>* %322, align 4, !tbaa !335
  %323 = fcmp ogt <4 x float> %wide.load.24, zeroinitializer
  %324 = fcmp ogt <4 x float> %wide.load8.24, zeroinitializer
  %325 = select <4 x i1> %323, <4 x float> %wide.load.24, <4 x float> zeroinitializer
  %326 = select <4 x i1> %324, <4 x float> %wide.load8.24, <4 x float> zeroinitializer
  %327 = getelementptr inbounds float, float* %3, i64 %318
  %328 = bitcast float* %327 to <4 x float>*
  store <4 x float> %325, <4 x float>* %328, align 4, !tbaa !338
  %329 = getelementptr inbounds float, float* %327, i64 4
  %330 = bitcast float* %329 to <4 x float>*
  store <4 x float> %326, <4 x float>* %330, align 4, !tbaa !338
  %331 = add nuw nsw i64 %6, 200
  %332 = getelementptr inbounds float, float* %2, i64 %331
  %333 = bitcast float* %332 to <4 x float>*
  %wide.load.25 = load <4 x float>, <4 x float>* %333, align 4, !tbaa !335
  %334 = getelementptr inbounds float, float* %332, i64 4
  %335 = bitcast float* %334 to <4 x float>*
  %wide.load8.25 = load <4 x float>, <4 x float>* %335, align 4, !tbaa !335
  %336 = fcmp ogt <4 x float> %wide.load.25, zeroinitializer
  %337 = fcmp ogt <4 x float> %wide.load8.25, zeroinitializer
  %338 = select <4 x i1> %336, <4 x float> %wide.load.25, <4 x float> zeroinitializer
  %339 = select <4 x i1> %337, <4 x float> %wide.load8.25, <4 x float> zeroinitializer
  %340 = getelementptr inbounds float, float* %3, i64 %331
  %341 = bitcast float* %340 to <4 x float>*
  store <4 x float> %338, <4 x float>* %341, align 4, !tbaa !338
  %342 = getelementptr inbounds float, float* %340, i64 4
  %343 = bitcast float* %342 to <4 x float>*
  store <4 x float> %339, <4 x float>* %343, align 4, !tbaa !338
  %344 = add nuw nsw i64 %6, 208
  %345 = getelementptr inbounds float, float* %2, i64 %344
  %346 = bitcast float* %345 to <4 x float>*
  %wide.load.26 = load <4 x float>, <4 x float>* %346, align 4, !tbaa !335
  %347 = getelementptr inbounds float, float* %345, i64 4
  %348 = bitcast float* %347 to <4 x float>*
  %wide.load8.26 = load <4 x float>, <4 x float>* %348, align 4, !tbaa !335
  %349 = fcmp ogt <4 x float> %wide.load.26, zeroinitializer
  %350 = fcmp ogt <4 x float> %wide.load8.26, zeroinitializer
  %351 = select <4 x i1> %349, <4 x float> %wide.load.26, <4 x float> zeroinitializer
  %352 = select <4 x i1> %350, <4 x float> %wide.load8.26, <4 x float> zeroinitializer
  %353 = getelementptr inbounds float, float* %3, i64 %344
  %354 = bitcast float* %353 to <4 x float>*
  store <4 x float> %351, <4 x float>* %354, align 4, !tbaa !338
  %355 = getelementptr inbounds float, float* %353, i64 4
  %356 = bitcast float* %355 to <4 x float>*
  store <4 x float> %352, <4 x float>* %356, align 4, !tbaa !338
  %357 = add nuw nsw i64 %6, 216
  %358 = getelementptr inbounds float, float* %2, i64 %357
  %359 = bitcast float* %358 to <4 x float>*
  %wide.load.27 = load <4 x float>, <4 x float>* %359, align 4, !tbaa !335
  %360 = getelementptr inbounds float, float* %358, i64 4
  %361 = bitcast float* %360 to <4 x float>*
  %wide.load8.27 = load <4 x float>, <4 x float>* %361, align 4, !tbaa !335
  %362 = fcmp ogt <4 x float> %wide.load.27, zeroinitializer
  %363 = fcmp ogt <4 x float> %wide.load8.27, zeroinitializer
  %364 = select <4 x i1> %362, <4 x float> %wide.load.27, <4 x float> zeroinitializer
  %365 = select <4 x i1> %363, <4 x float> %wide.load8.27, <4 x float> zeroinitializer
  %366 = getelementptr inbounds float, float* %3, i64 %357
  %367 = bitcast float* %366 to <4 x float>*
  store <4 x float> %364, <4 x float>* %367, align 4, !tbaa !338
  %368 = getelementptr inbounds float, float* %366, i64 4
  %369 = bitcast float* %368 to <4 x float>*
  store <4 x float> %365, <4 x float>* %369, align 4, !tbaa !338
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 224
  br i1 %exitcond3, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 64
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !34
}

; Function Attrs: norecurse nounwind
define dllexport i32 @fused_nn_max_pool2d_4(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 {
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
  tail call fastcc void @fused_nn_max_pool2d_4_compute_(i8* %11, i8* %9)
  ret i32 0
}

; Function Attrs: noinline norecurse nounwind
define private fastcc void @fused_nn_max_pool2d_4_compute_(i8* noalias nocapture, i8* noalias nocapture readonly) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %0 to float*
  %3 = bitcast i8* %1 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvars.iv6 = phi i64 [ 0, %entry ], [ %indvars.iv.next7, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv6, 12544
  %5 = mul nuw nsw i64 %indvars.iv6, 50176
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_end6, %for_begin1.preheader
  %indvars.iv3 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next4, %for_end6 ]
  %6 = mul nuw nsw i64 %indvars.iv3, 112
  %7 = add nuw nsw i64 %6, %4
  %8 = mul nuw nsw i64 %indvars.iv3, 448
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
  %wide.vec = load <8 x float>, <8 x float>* %15, align 4, !tbaa !341
  %strided.vec = shufflevector <8 x float> %wide.vec, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9 = shufflevector <8 x float> %wide.vec, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %16 = fcmp olt <4 x float> %strided.vec, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %17 = select <4 x i1> %16, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec
  %18 = fcmp ogt <4 x float> %17, %strided.vec9
  %19 = select <4 x i1> %18, <4 x float> %17, <4 x float> %strided.vec9
  %20 = add nuw nsw i64 %13, 224
  %21 = getelementptr inbounds float, float* %3, i64 %20
  %22 = bitcast float* %21 to <8 x float>*
  %wide.vec10 = load <8 x float>, <8 x float>* %22, align 4, !tbaa !341
  %strided.vec11 = shufflevector <8 x float> %wide.vec10, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12 = shufflevector <8 x float> %wide.vec10, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %23 = fcmp ogt <4 x float> %19, %strided.vec11
  %24 = select <4 x i1> %23, <4 x float> %19, <4 x float> %strided.vec11
  %25 = fcmp ogt <4 x float> %24, %strided.vec12
  %26 = select <4 x i1> %25, <4 x float> %24, <4 x float> %strided.vec12
  %27 = bitcast float* %11 to <4 x float>*
  store <4 x float> %26, <4 x float>* %27, align 4, !tbaa !344
  %index.next = add i64 %index, 4
  %28 = icmp eq i64 %index.next, 112
  br i1 %28, label %for_end6, label %vector.body, !llvm.loop !347

for_end3:                                         ; preds = %for_end6
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  %exitcond8 = icmp eq i64 %indvars.iv.next7, 64
  br i1 %exitcond8, label %for_end, label %for_begin1.preheader, !prof !34

for_end6:                                         ; preds = %vector.body
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 112
  br i1 %exitcond5, label %for_end3, label %for_begin4.preheader, !prof !34
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
  %4 = mul nuw nsw i64 %indvars.iv6, 784
  %5 = mul nuw nsw i64 %indvars.iv6, 3136
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv3 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next4, %for_begin4.preheader ]
  %6 = mul nuw nsw i64 %indvars.iv3, 28
  %7 = add nuw nsw i64 %6, %4
  %8 = mul nuw nsw i64 %indvars.iv3, 112
  %9 = add nuw nsw i64 %8, %5
  %10 = getelementptr inbounds float, float* %2, i64 %7
  %11 = getelementptr inbounds float, float* %3, i64 %9
  %12 = bitcast float* %11 to <8 x float>*
  %wide.vec = load <8 x float>, <8 x float>* %12, align 4, !tbaa !348
  %strided.vec = shufflevector <8 x float> %wide.vec, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9 = shufflevector <8 x float> %wide.vec, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %13 = fcmp olt <4 x float> %strided.vec, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %14 = select <4 x i1> %13, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec
  %15 = fcmp ogt <4 x float> %14, %strided.vec9
  %16 = select <4 x i1> %15, <4 x float> %14, <4 x float> %strided.vec9
  %17 = add nuw nsw i64 %9, 56
  %18 = getelementptr inbounds float, float* %3, i64 %17
  %19 = bitcast float* %18 to <8 x float>*
  %wide.vec10 = load <8 x float>, <8 x float>* %19, align 4, !tbaa !348
  %strided.vec11 = shufflevector <8 x float> %wide.vec10, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12 = shufflevector <8 x float> %wide.vec10, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %20 = fcmp ogt <4 x float> %16, %strided.vec11
  %21 = select <4 x i1> %20, <4 x float> %16, <4 x float> %strided.vec11
  %22 = fcmp ogt <4 x float> %21, %strided.vec12
  %23 = select <4 x i1> %22, <4 x float> %21, <4 x float> %strided.vec12
  %24 = bitcast float* %10 to <4 x float>*
  store <4 x float> %23, <4 x float>* %24, align 4, !tbaa !351
  %25 = add nuw nsw i64 %7, 4
  %26 = getelementptr inbounds float, float* %2, i64 %25
  %27 = or i64 %9, 8
  %28 = getelementptr inbounds float, float* %3, i64 %27
  %29 = bitcast float* %28 to <8 x float>*
  %wide.vec.1 = load <8 x float>, <8 x float>* %29, align 4, !tbaa !348
  %strided.vec.1 = shufflevector <8 x float> %wide.vec.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.1 = shufflevector <8 x float> %wide.vec.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %30 = fcmp olt <4 x float> %strided.vec.1, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %31 = select <4 x i1> %30, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.1
  %32 = fcmp ogt <4 x float> %31, %strided.vec9.1
  %33 = select <4 x i1> %32, <4 x float> %31, <4 x float> %strided.vec9.1
  %34 = add nuw nsw i64 %27, 56
  %35 = getelementptr inbounds float, float* %3, i64 %34
  %36 = bitcast float* %35 to <8 x float>*
  %wide.vec10.1 = load <8 x float>, <8 x float>* %36, align 4, !tbaa !348
  %strided.vec11.1 = shufflevector <8 x float> %wide.vec10.1, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.1 = shufflevector <8 x float> %wide.vec10.1, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %37 = fcmp ogt <4 x float> %33, %strided.vec11.1
  %38 = select <4 x i1> %37, <4 x float> %33, <4 x float> %strided.vec11.1
  %39 = fcmp ogt <4 x float> %38, %strided.vec12.1
  %40 = select <4 x i1> %39, <4 x float> %38, <4 x float> %strided.vec12.1
  %41 = bitcast float* %26 to <4 x float>*
  store <4 x float> %40, <4 x float>* %41, align 4, !tbaa !351
  %42 = add nuw nsw i64 %7, 8
  %43 = getelementptr inbounds float, float* %2, i64 %42
  %44 = add nuw nsw i64 %9, 16
  %45 = getelementptr inbounds float, float* %3, i64 %44
  %46 = bitcast float* %45 to <8 x float>*
  %wide.vec.2 = load <8 x float>, <8 x float>* %46, align 4, !tbaa !348
  %strided.vec.2 = shufflevector <8 x float> %wide.vec.2, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.2 = shufflevector <8 x float> %wide.vec.2, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %47 = fcmp olt <4 x float> %strided.vec.2, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %48 = select <4 x i1> %47, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.2
  %49 = fcmp ogt <4 x float> %48, %strided.vec9.2
  %50 = select <4 x i1> %49, <4 x float> %48, <4 x float> %strided.vec9.2
  %51 = add nuw nsw i64 %9, 72
  %52 = getelementptr inbounds float, float* %3, i64 %51
  %53 = bitcast float* %52 to <8 x float>*
  %wide.vec10.2 = load <8 x float>, <8 x float>* %53, align 4, !tbaa !348
  %strided.vec11.2 = shufflevector <8 x float> %wide.vec10.2, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.2 = shufflevector <8 x float> %wide.vec10.2, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %54 = fcmp ogt <4 x float> %50, %strided.vec11.2
  %55 = select <4 x i1> %54, <4 x float> %50, <4 x float> %strided.vec11.2
  %56 = fcmp ogt <4 x float> %55, %strided.vec12.2
  %57 = select <4 x i1> %56, <4 x float> %55, <4 x float> %strided.vec12.2
  %58 = bitcast float* %43 to <4 x float>*
  store <4 x float> %57, <4 x float>* %58, align 4, !tbaa !351
  %59 = add nuw nsw i64 %7, 12
  %60 = getelementptr inbounds float, float* %2, i64 %59
  %61 = add nuw nsw i64 %9, 24
  %62 = getelementptr inbounds float, float* %3, i64 %61
  %63 = bitcast float* %62 to <8 x float>*
  %wide.vec.3 = load <8 x float>, <8 x float>* %63, align 4, !tbaa !348
  %strided.vec.3 = shufflevector <8 x float> %wide.vec.3, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.3 = shufflevector <8 x float> %wide.vec.3, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %64 = fcmp olt <4 x float> %strided.vec.3, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %65 = select <4 x i1> %64, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.3
  %66 = fcmp ogt <4 x float> %65, %strided.vec9.3
  %67 = select <4 x i1> %66, <4 x float> %65, <4 x float> %strided.vec9.3
  %68 = add nuw nsw i64 %9, 80
  %69 = getelementptr inbounds float, float* %3, i64 %68
  %70 = bitcast float* %69 to <8 x float>*
  %wide.vec10.3 = load <8 x float>, <8 x float>* %70, align 4, !tbaa !348
  %strided.vec11.3 = shufflevector <8 x float> %wide.vec10.3, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.3 = shufflevector <8 x float> %wide.vec10.3, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %71 = fcmp ogt <4 x float> %67, %strided.vec11.3
  %72 = select <4 x i1> %71, <4 x float> %67, <4 x float> %strided.vec11.3
  %73 = fcmp ogt <4 x float> %72, %strided.vec12.3
  %74 = select <4 x i1> %73, <4 x float> %72, <4 x float> %strided.vec12.3
  %75 = bitcast float* %60 to <4 x float>*
  store <4 x float> %74, <4 x float>* %75, align 4, !tbaa !351
  %76 = add nuw nsw i64 %7, 16
  %77 = getelementptr inbounds float, float* %2, i64 %76
  %78 = add nuw nsw i64 %9, 32
  %79 = getelementptr inbounds float, float* %3, i64 %78
  %80 = bitcast float* %79 to <8 x float>*
  %wide.vec.4 = load <8 x float>, <8 x float>* %80, align 4, !tbaa !348
  %strided.vec.4 = shufflevector <8 x float> %wide.vec.4, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.4 = shufflevector <8 x float> %wide.vec.4, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %81 = fcmp olt <4 x float> %strided.vec.4, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %82 = select <4 x i1> %81, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.4
  %83 = fcmp ogt <4 x float> %82, %strided.vec9.4
  %84 = select <4 x i1> %83, <4 x float> %82, <4 x float> %strided.vec9.4
  %85 = add nuw nsw i64 %9, 88
  %86 = getelementptr inbounds float, float* %3, i64 %85
  %87 = bitcast float* %86 to <8 x float>*
  %wide.vec10.4 = load <8 x float>, <8 x float>* %87, align 4, !tbaa !348
  %strided.vec11.4 = shufflevector <8 x float> %wide.vec10.4, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.4 = shufflevector <8 x float> %wide.vec10.4, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %88 = fcmp ogt <4 x float> %84, %strided.vec11.4
  %89 = select <4 x i1> %88, <4 x float> %84, <4 x float> %strided.vec11.4
  %90 = fcmp ogt <4 x float> %89, %strided.vec12.4
  %91 = select <4 x i1> %90, <4 x float> %89, <4 x float> %strided.vec12.4
  %92 = bitcast float* %77 to <4 x float>*
  store <4 x float> %91, <4 x float>* %92, align 4, !tbaa !351
  %93 = add nuw nsw i64 %7, 20
  %94 = getelementptr inbounds float, float* %2, i64 %93
  %95 = add nuw nsw i64 %9, 40
  %96 = getelementptr inbounds float, float* %3, i64 %95
  %97 = bitcast float* %96 to <8 x float>*
  %wide.vec.5 = load <8 x float>, <8 x float>* %97, align 4, !tbaa !348
  %strided.vec.5 = shufflevector <8 x float> %wide.vec.5, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.5 = shufflevector <8 x float> %wide.vec.5, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %98 = fcmp olt <4 x float> %strided.vec.5, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %99 = select <4 x i1> %98, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.5
  %100 = fcmp ogt <4 x float> %99, %strided.vec9.5
  %101 = select <4 x i1> %100, <4 x float> %99, <4 x float> %strided.vec9.5
  %102 = add nuw nsw i64 %9, 96
  %103 = getelementptr inbounds float, float* %3, i64 %102
  %104 = bitcast float* %103 to <8 x float>*
  %wide.vec10.5 = load <8 x float>, <8 x float>* %104, align 4, !tbaa !348
  %strided.vec11.5 = shufflevector <8 x float> %wide.vec10.5, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.5 = shufflevector <8 x float> %wide.vec10.5, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %105 = fcmp ogt <4 x float> %101, %strided.vec11.5
  %106 = select <4 x i1> %105, <4 x float> %101, <4 x float> %strided.vec11.5
  %107 = fcmp ogt <4 x float> %106, %strided.vec12.5
  %108 = select <4 x i1> %107, <4 x float> %106, <4 x float> %strided.vec12.5
  %109 = bitcast float* %94 to <4 x float>*
  store <4 x float> %108, <4 x float>* %109, align 4, !tbaa !351
  %110 = add nuw nsw i64 %7, 24
  %111 = getelementptr inbounds float, float* %2, i64 %110
  %112 = add nuw nsw i64 %9, 48
  %113 = getelementptr inbounds float, float* %3, i64 %112
  %114 = bitcast float* %113 to <8 x float>*
  %wide.vec.6 = load <8 x float>, <8 x float>* %114, align 4, !tbaa !348
  %strided.vec.6 = shufflevector <8 x float> %wide.vec.6, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec9.6 = shufflevector <8 x float> %wide.vec.6, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %115 = fcmp olt <4 x float> %strided.vec.6, <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>
  %116 = select <4 x i1> %115, <4 x float> <float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000, float 0xC7EFFFFFE0000000>, <4 x float> %strided.vec.6
  %117 = fcmp ogt <4 x float> %116, %strided.vec9.6
  %118 = select <4 x i1> %117, <4 x float> %116, <4 x float> %strided.vec9.6
  %119 = add nuw nsw i64 %9, 104
  %120 = getelementptr inbounds float, float* %3, i64 %119
  %121 = bitcast float* %120 to <8 x float>*
  %wide.vec10.6 = load <8 x float>, <8 x float>* %121, align 4, !tbaa !348
  %strided.vec11.6 = shufflevector <8 x float> %wide.vec10.6, <8 x float> undef, <4 x i32> <i32 0, i32 2, i32 4, i32 6>
  %strided.vec12.6 = shufflevector <8 x float> %wide.vec10.6, <8 x float> undef, <4 x i32> <i32 1, i32 3, i32 5, i32 7>
  %122 = fcmp ogt <4 x float> %118, %strided.vec11.6
  %123 = select <4 x i1> %122, <4 x float> %118, <4 x float> %strided.vec11.6
  %124 = fcmp ogt <4 x float> %123, %strided.vec12.6
  %125 = select <4 x i1> %124, <4 x float> %123, <4 x float> %strided.vec12.6
  %126 = bitcast float* %111 to <4 x float>*
  store <4 x float> %125, <4 x float>* %126, align 4, !tbaa !351
  %indvars.iv.next4 = add nuw nsw i64 %indvars.iv3, 1
  %exitcond5 = icmp eq i64 %indvars.iv.next4, 28
  br i1 %exitcond5, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next7 = add nuw nsw i64 %indvars.iv6, 1
  %exitcond8 = icmp eq i64 %indvars.iv.next7, 256
  br i1 %exitcond8, label %for_end, label %for_begin1.preheader, !prof !34
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
  %indvars.iv4 = phi i64 [ 0, %entry ], [ %indvars.iv.next5, %for_end3 ]
  %4 = mul nuw nsw i64 %indvars.iv4, 3136
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv1 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next2, %for_begin4.preheader ]
  %5 = mul nuw nsw i64 %indvars.iv1, 56
  %6 = add nuw nsw i64 %5, %4
  %7 = getelementptr inbounds float, float* %2, i64 %6
  %8 = bitcast float* %7 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %8, align 4, !tbaa !354
  %9 = fcmp ogt <4 x float> %wide.load, zeroinitializer
  %10 = select <4 x i1> %9, <4 x float> %wide.load, <4 x float> zeroinitializer
  %11 = getelementptr inbounds float, float* %3, i64 %6
  %12 = bitcast float* %11 to <4 x float>*
  store <4 x float> %10, <4 x float>* %12, align 4, !tbaa !357
  %13 = or i64 %6, 4
  %14 = getelementptr inbounds float, float* %2, i64 %13
  %15 = bitcast float* %14 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %15, align 4, !tbaa !354
  %16 = fcmp ogt <4 x float> %wide.load.1, zeroinitializer
  %17 = select <4 x i1> %16, <4 x float> %wide.load.1, <4 x float> zeroinitializer
  %18 = getelementptr inbounds float, float* %3, i64 %13
  %19 = bitcast float* %18 to <4 x float>*
  store <4 x float> %17, <4 x float>* %19, align 4, !tbaa !357
  %20 = add nuw nsw i64 %6, 8
  %21 = getelementptr inbounds float, float* %2, i64 %20
  %22 = bitcast float* %21 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %22, align 4, !tbaa !354
  %23 = fcmp ogt <4 x float> %wide.load.2, zeroinitializer
  %24 = select <4 x i1> %23, <4 x float> %wide.load.2, <4 x float> zeroinitializer
  %25 = getelementptr inbounds float, float* %3, i64 %20
  %26 = bitcast float* %25 to <4 x float>*
  store <4 x float> %24, <4 x float>* %26, align 4, !tbaa !357
  %27 = add nuw nsw i64 %6, 12
  %28 = getelementptr inbounds float, float* %2, i64 %27
  %29 = bitcast float* %28 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %29, align 4, !tbaa !354
  %30 = fcmp ogt <4 x float> %wide.load.3, zeroinitializer
  %31 = select <4 x i1> %30, <4 x float> %wide.load.3, <4 x float> zeroinitializer
  %32 = getelementptr inbounds float, float* %3, i64 %27
  %33 = bitcast float* %32 to <4 x float>*
  store <4 x float> %31, <4 x float>* %33, align 4, !tbaa !357
  %34 = add nuw nsw i64 %6, 16
  %35 = getelementptr inbounds float, float* %2, i64 %34
  %36 = bitcast float* %35 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %36, align 4, !tbaa !354
  %37 = fcmp ogt <4 x float> %wide.load.4, zeroinitializer
  %38 = select <4 x i1> %37, <4 x float> %wide.load.4, <4 x float> zeroinitializer
  %39 = getelementptr inbounds float, float* %3, i64 %34
  %40 = bitcast float* %39 to <4 x float>*
  store <4 x float> %38, <4 x float>* %40, align 4, !tbaa !357
  %41 = add nuw nsw i64 %6, 20
  %42 = getelementptr inbounds float, float* %2, i64 %41
  %43 = bitcast float* %42 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %43, align 4, !tbaa !354
  %44 = fcmp ogt <4 x float> %wide.load.5, zeroinitializer
  %45 = select <4 x i1> %44, <4 x float> %wide.load.5, <4 x float> zeroinitializer
  %46 = getelementptr inbounds float, float* %3, i64 %41
  %47 = bitcast float* %46 to <4 x float>*
  store <4 x float> %45, <4 x float>* %47, align 4, !tbaa !357
  %48 = add nuw nsw i64 %6, 24
  %49 = getelementptr inbounds float, float* %2, i64 %48
  %50 = bitcast float* %49 to <4 x float>*
  %wide.load.6 = load <4 x float>, <4 x float>* %50, align 4, !tbaa !354
  %51 = fcmp ogt <4 x float> %wide.load.6, zeroinitializer
  %52 = select <4 x i1> %51, <4 x float> %wide.load.6, <4 x float> zeroinitializer
  %53 = getelementptr inbounds float, float* %3, i64 %48
  %54 = bitcast float* %53 to <4 x float>*
  store <4 x float> %52, <4 x float>* %54, align 4, !tbaa !357
  %55 = add nuw nsw i64 %6, 28
  %56 = getelementptr inbounds float, float* %2, i64 %55
  %57 = bitcast float* %56 to <4 x float>*
  %wide.load.7 = load <4 x float>, <4 x float>* %57, align 4, !tbaa !354
  %58 = fcmp ogt <4 x float> %wide.load.7, zeroinitializer
  %59 = select <4 x i1> %58, <4 x float> %wide.load.7, <4 x float> zeroinitializer
  %60 = getelementptr inbounds float, float* %3, i64 %55
  %61 = bitcast float* %60 to <4 x float>*
  store <4 x float> %59, <4 x float>* %61, align 4, !tbaa !357
  %62 = add nuw nsw i64 %6, 32
  %63 = getelementptr inbounds float, float* %2, i64 %62
  %64 = bitcast float* %63 to <4 x float>*
  %wide.load.8 = load <4 x float>, <4 x float>* %64, align 4, !tbaa !354
  %65 = fcmp ogt <4 x float> %wide.load.8, zeroinitializer
  %66 = select <4 x i1> %65, <4 x float> %wide.load.8, <4 x float> zeroinitializer
  %67 = getelementptr inbounds float, float* %3, i64 %62
  %68 = bitcast float* %67 to <4 x float>*
  store <4 x float> %66, <4 x float>* %68, align 4, !tbaa !357
  %69 = add nuw nsw i64 %6, 36
  %70 = getelementptr inbounds float, float* %2, i64 %69
  %71 = bitcast float* %70 to <4 x float>*
  %wide.load.9 = load <4 x float>, <4 x float>* %71, align 4, !tbaa !354
  %72 = fcmp ogt <4 x float> %wide.load.9, zeroinitializer
  %73 = select <4 x i1> %72, <4 x float> %wide.load.9, <4 x float> zeroinitializer
  %74 = getelementptr inbounds float, float* %3, i64 %69
  %75 = bitcast float* %74 to <4 x float>*
  store <4 x float> %73, <4 x float>* %75, align 4, !tbaa !357
  %76 = add nuw nsw i64 %6, 40
  %77 = getelementptr inbounds float, float* %2, i64 %76
  %78 = bitcast float* %77 to <4 x float>*
  %wide.load.10 = load <4 x float>, <4 x float>* %78, align 4, !tbaa !354
  %79 = fcmp ogt <4 x float> %wide.load.10, zeroinitializer
  %80 = select <4 x i1> %79, <4 x float> %wide.load.10, <4 x float> zeroinitializer
  %81 = getelementptr inbounds float, float* %3, i64 %76
  %82 = bitcast float* %81 to <4 x float>*
  store <4 x float> %80, <4 x float>* %82, align 4, !tbaa !357
  %83 = add nuw nsw i64 %6, 44
  %84 = getelementptr inbounds float, float* %2, i64 %83
  %85 = bitcast float* %84 to <4 x float>*
  %wide.load.11 = load <4 x float>, <4 x float>* %85, align 4, !tbaa !354
  %86 = fcmp ogt <4 x float> %wide.load.11, zeroinitializer
  %87 = select <4 x i1> %86, <4 x float> %wide.load.11, <4 x float> zeroinitializer
  %88 = getelementptr inbounds float, float* %3, i64 %83
  %89 = bitcast float* %88 to <4 x float>*
  store <4 x float> %87, <4 x float>* %89, align 4, !tbaa !357
  %90 = add nuw nsw i64 %6, 48
  %91 = getelementptr inbounds float, float* %2, i64 %90
  %92 = bitcast float* %91 to <4 x float>*
  %wide.load.12 = load <4 x float>, <4 x float>* %92, align 4, !tbaa !354
  %93 = fcmp ogt <4 x float> %wide.load.12, zeroinitializer
  %94 = select <4 x i1> %93, <4 x float> %wide.load.12, <4 x float> zeroinitializer
  %95 = getelementptr inbounds float, float* %3, i64 %90
  %96 = bitcast float* %95 to <4 x float>*
  store <4 x float> %94, <4 x float>* %96, align 4, !tbaa !357
  %97 = add nuw nsw i64 %6, 52
  %98 = getelementptr inbounds float, float* %2, i64 %97
  %99 = bitcast float* %98 to <4 x float>*
  %wide.load.13 = load <4 x float>, <4 x float>* %99, align 4, !tbaa !354
  %100 = fcmp ogt <4 x float> %wide.load.13, zeroinitializer
  %101 = select <4 x i1> %100, <4 x float> %wide.load.13, <4 x float> zeroinitializer
  %102 = getelementptr inbounds float, float* %3, i64 %97
  %103 = bitcast float* %102 to <4 x float>*
  store <4 x float> %101, <4 x float>* %103, align 4, !tbaa !357
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 56
  br i1 %exitcond3, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 256
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !34
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
  %indvars.iv4 = phi i64 [ 0, %entry ], [ %indvars.iv.next5, %for_end3 ]
  %6 = mul nuw nsw i64 %indvars.iv4, 3136
  %7 = getelementptr inbounds float, float* %3, i64 %indvars.iv4
  %8 = load float, float* %7, align 4, !tbaa !360
  %broadcast.splatinsert7 = insertelement <4 x float> undef, float %8, i32 0
  %broadcast.splat8 = shufflevector <4 x float> %broadcast.splatinsert7, <4 x float> undef, <4 x i32> zeroinitializer
  br label %for_begin4.preheader

for_end:                                          ; preds = %for_end3
  ret void

for_begin4.preheader:                             ; preds = %for_begin4.preheader, %for_begin1.preheader
  %indvars.iv1 = phi i64 [ 0, %for_begin1.preheader ], [ %indvars.iv.next2, %for_begin4.preheader ]
  %9 = mul nuw nsw i64 %indvars.iv1, 56
  %10 = add nuw nsw i64 %9, %6
  %11 = getelementptr inbounds float, float* %4, i64 %10
  %12 = bitcast float* %11 to <4 x float>*
  %wide.load = load <4 x float>, <4 x float>* %12, align 4, !tbaa !363
  %13 = fadd <4 x float> %broadcast.splat8, %wide.load
  %14 = getelementptr inbounds float, float* %5, i64 %10
  %15 = bitcast float* %14 to <4 x float>*
  store <4 x float> %13, <4 x float>* %15, align 4, !tbaa !366
  %16 = or i64 %10, 4
  %17 = getelementptr inbounds float, float* %4, i64 %16
  %18 = bitcast float* %17 to <4 x float>*
  %wide.load.1 = load <4 x float>, <4 x float>* %18, align 4, !tbaa !363
  %19 = fadd <4 x float> %broadcast.splat8, %wide.load.1
  %20 = getelementptr inbounds float, float* %5, i64 %16
  %21 = bitcast float* %20 to <4 x float>*
  store <4 x float> %19, <4 x float>* %21, align 4, !tbaa !366
  %22 = add nuw nsw i64 %10, 8
  %23 = getelementptr inbounds float, float* %4, i64 %22
  %24 = bitcast float* %23 to <4 x float>*
  %wide.load.2 = load <4 x float>, <4 x float>* %24, align 4, !tbaa !363
  %25 = fadd <4 x float> %broadcast.splat8, %wide.load.2
  %26 = getelementptr inbounds float, float* %5, i64 %22
  %27 = bitcast float* %26 to <4 x float>*
  store <4 x float> %25, <4 x float>* %27, align 4, !tbaa !366
  %28 = add nuw nsw i64 %10, 12
  %29 = getelementptr inbounds float, float* %4, i64 %28
  %30 = bitcast float* %29 to <4 x float>*
  %wide.load.3 = load <4 x float>, <4 x float>* %30, align 4, !tbaa !363
  %31 = fadd <4 x float> %broadcast.splat8, %wide.load.3
  %32 = getelementptr inbounds float, float* %5, i64 %28
  %33 = bitcast float* %32 to <4 x float>*
  store <4 x float> %31, <4 x float>* %33, align 4, !tbaa !366
  %34 = add nuw nsw i64 %10, 16
  %35 = getelementptr inbounds float, float* %4, i64 %34
  %36 = bitcast float* %35 to <4 x float>*
  %wide.load.4 = load <4 x float>, <4 x float>* %36, align 4, !tbaa !363
  %37 = fadd <4 x float> %broadcast.splat8, %wide.load.4
  %38 = getelementptr inbounds float, float* %5, i64 %34
  %39 = bitcast float* %38 to <4 x float>*
  store <4 x float> %37, <4 x float>* %39, align 4, !tbaa !366
  %40 = add nuw nsw i64 %10, 20
  %41 = getelementptr inbounds float, float* %4, i64 %40
  %42 = bitcast float* %41 to <4 x float>*
  %wide.load.5 = load <4 x float>, <4 x float>* %42, align 4, !tbaa !363
  %43 = fadd <4 x float> %broadcast.splat8, %wide.load.5
  %44 = getelementptr inbounds float, float* %5, i64 %40
  %45 = bitcast float* %44 to <4 x float>*
  store <4 x float> %43, <4 x float>* %45, align 4, !tbaa !366
  %46 = add nuw nsw i64 %10, 24
  %47 = getelementptr inbounds float, float* %4, i64 %46
  %48 = bitcast float* %47 to <4 x float>*
  %wide.load.6 = load <4 x float>, <4 x float>* %48, align 4, !tbaa !363
  %49 = fadd <4 x float> %broadcast.splat8, %wide.load.6
  %50 = getelementptr inbounds float, float* %5, i64 %46
  %51 = bitcast float* %50 to <4 x float>*
  store <4 x float> %49, <4 x float>* %51, align 4, !tbaa !366
  %52 = add nuw nsw i64 %10, 28
  %53 = getelementptr inbounds float, float* %4, i64 %52
  %54 = bitcast float* %53 to <4 x float>*
  %wide.load.7 = load <4 x float>, <4 x float>* %54, align 4, !tbaa !363
  %55 = fadd <4 x float> %broadcast.splat8, %wide.load.7
  %56 = getelementptr inbounds float, float* %5, i64 %52
  %57 = bitcast float* %56 to <4 x float>*
  store <4 x float> %55, <4 x float>* %57, align 4, !tbaa !366
  %58 = add nuw nsw i64 %10, 32
  %59 = getelementptr inbounds float, float* %4, i64 %58
  %60 = bitcast float* %59 to <4 x float>*
  %wide.load.8 = load <4 x float>, <4 x float>* %60, align 4, !tbaa !363
  %61 = fadd <4 x float> %broadcast.splat8, %wide.load.8
  %62 = getelementptr inbounds float, float* %5, i64 %58
  %63 = bitcast float* %62 to <4 x float>*
  store <4 x float> %61, <4 x float>* %63, align 4, !tbaa !366
  %64 = add nuw nsw i64 %10, 36
  %65 = getelementptr inbounds float, float* %4, i64 %64
  %66 = bitcast float* %65 to <4 x float>*
  %wide.load.9 = load <4 x float>, <4 x float>* %66, align 4, !tbaa !363
  %67 = fadd <4 x float> %broadcast.splat8, %wide.load.9
  %68 = getelementptr inbounds float, float* %5, i64 %64
  %69 = bitcast float* %68 to <4 x float>*
  store <4 x float> %67, <4 x float>* %69, align 4, !tbaa !366
  %70 = add nuw nsw i64 %10, 40
  %71 = getelementptr inbounds float, float* %4, i64 %70
  %72 = bitcast float* %71 to <4 x float>*
  %wide.load.10 = load <4 x float>, <4 x float>* %72, align 4, !tbaa !363
  %73 = fadd <4 x float> %broadcast.splat8, %wide.load.10
  %74 = getelementptr inbounds float, float* %5, i64 %70
  %75 = bitcast float* %74 to <4 x float>*
  store <4 x float> %73, <4 x float>* %75, align 4, !tbaa !366
  %76 = add nuw nsw i64 %10, 44
  %77 = getelementptr inbounds float, float* %4, i64 %76
  %78 = bitcast float* %77 to <4 x float>*
  %wide.load.11 = load <4 x float>, <4 x float>* %78, align 4, !tbaa !363
  %79 = fadd <4 x float> %broadcast.splat8, %wide.load.11
  %80 = getelementptr inbounds float, float* %5, i64 %76
  %81 = bitcast float* %80 to <4 x float>*
  store <4 x float> %79, <4 x float>* %81, align 4, !tbaa !366
  %82 = add nuw nsw i64 %10, 48
  %83 = getelementptr inbounds float, float* %4, i64 %82
  %84 = bitcast float* %83 to <4 x float>*
  %wide.load.12 = load <4 x float>, <4 x float>* %84, align 4, !tbaa !363
  %85 = fadd <4 x float> %broadcast.splat8, %wide.load.12
  %86 = getelementptr inbounds float, float* %5, i64 %82
  %87 = bitcast float* %86 to <4 x float>*
  store <4 x float> %85, <4 x float>* %87, align 4, !tbaa !366
  %88 = add nuw nsw i64 %10, 52
  %89 = getelementptr inbounds float, float* %4, i64 %88
  %90 = bitcast float* %89 to <4 x float>*
  %wide.load.13 = load <4 x float>, <4 x float>* %90, align 4, !tbaa !363
  %91 = fadd <4 x float> %broadcast.splat8, %wide.load.13
  %92 = getelementptr inbounds float, float* %5, i64 %88
  %93 = bitcast float* %92 to <4 x float>*
  store <4 x float> %91, <4 x float>* %93, align 4, !tbaa !366
  %indvars.iv.next2 = add nuw nsw i64 %indvars.iv1, 1
  %exitcond3 = icmp eq i64 %indvars.iv.next2, 56
  br i1 %exitcond3, label %for_end3, label %for_begin4.preheader, !prof !34

for_end3:                                         ; preds = %for_begin4.preheader
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 256
  br i1 %exitcond6, label %for_end, label %for_begin1.preheader, !prof !34
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_conv2d_7(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
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
  tail call fastcc void @fused_nn_conv2d_7_compute_(i8* %12, i8* %14, i8* %16)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_conv2d_7_compute_(i8* noalias nocapture readonly, i8* noalias readonly, i8* noalias) unnamed_addr #3 {
entry:
  %3 = alloca [8 x <32 x float>], align 16
  %4 = alloca [224 x <32 x float>], align 16
  %5 = alloca [1152 x <32 x float>], align 16
  %6 = alloca [3268864 x float], align 16
  %7 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar93 = phi i64 [ 0, %entry ], [ %indvar.next94, %for_end3 ]
  %8 = mul nuw nsw i64 %indvar93, 3616
  %9 = trunc i64 %indvar93 to i32
  %10 = urem i32 %9, 226
  %11 = udiv i32 %9, 226
  %.off = add nsw i32 %10, -1
  %12 = icmp ult i32 %.off, 224
  br i1 %12, label %for_begin4.preheader.us.preheader, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep100 = getelementptr [3268864 x float], [3268864 x float]* %6, i64 0, i64 %8
  %scevgep100101 = bitcast float* %scevgep100 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep100101, i8 0, i64 14464, i1 false)
  br label %for_end3

for_begin4.preheader.us.preheader:                ; preds = %for_begin1.preheader
  %13 = mul nsw i32 %11, 802816
  %14 = add nsw i32 %13, -225
  %15 = mul nuw nsw i32 %10, 224
  %16 = add i32 %14, %15
  %17 = sext i32 %16 to i64
  br label %for_body5.us.us

for_body5.us.us:                                  ; preds = %if_end.us.us, %for_begin4.preheader.us.preheader
  %indvars.iv102 = phi i64 [ 0, %for_begin4.preheader.us.preheader ], [ %indvars.iv.next103, %if_end.us.us ]
  %18 = phi i32 [ 0, %for_begin4.preheader.us.preheader ], [ %25, %if_end.us.us ]
  %19 = add nuw nsw i64 %8, %indvars.iv102
  %trunc.us.us = trunc i32 %18 to i31
  switch i31 %trunc.us.us, label %if_then.us.us [
    i31 225, label %if_end.us.us
    i31 0, label %if_end.us.us
  ]

if_then.us.us:                                    ; preds = %for_body5.us.us
  %20 = add nsw i64 %indvars.iv102, %17
  %21 = getelementptr inbounds float, float* %7, i64 %20
  %22 = load float, float* %21, align 4, !tbaa !369
  br label %if_end.us.us

if_end.us.us:                                     ; preds = %if_then.us.us, %for_body5.us.us, %for_body5.us.us
  %23 = phi float [ %22, %if_then.us.us ], [ 0.000000e+00, %for_body5.us.us ], [ 0.000000e+00, %for_body5.us.us ]
  %24 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %19
  store float %23, float* %24, align 4, !tbaa !372
  %indvars.iv.next103 = add nuw nsw i64 %indvars.iv102, 1
  %25 = add nuw nsw i32 %18, 1
  %exitcond104 = icmp eq i64 %indvars.iv.next103, 226
  br i1 %exitcond104, label %for_end6.us-lcssa.us.us, label %for_body5.us.us, !prof !34

for_end6.us-lcssa.us.us:                          ; preds = %if_end.us.us
  %26 = add nuw nsw i64 %8, 226
  %27 = add nsw i64 %17, 50176
  br label %for_body5.us.us.1

for_begin7.preheader:                             ; preds = %for_end3
  %28 = bitcast [8 x <32 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0
  %29 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_end3:                                         ; preds = %if_end.us.us.15, %for_begin4.preheader.preheader
  %indvar.next94 = add nuw nsw i64 %indvar93, 1
  %exitcond108 = icmp eq i64 %indvar.next94, 904
  br i1 %exitcond108, label %for_begin7.preheader, label %for_begin1.preheader, !prof !34

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %30 = phi i32 [ 0, %for_begin7.preheader ], [ %60, %for_end12 ]
  %31 = urem i32 %30, 3
  %32 = mul nuw nsw i32 %31, 1536
  %33 = udiv i32 %30, 3
  %34 = mul nsw i32 %33, 18432
  %35 = add nuw i32 %32, %34
  %36 = mul nuw nsw i32 %31, 3
  %37 = or i32 %36, %34
  %38 = zext i32 %37 to i64
  %39 = sext i32 %35 to i64
  br label %for_begin13.preheader

for_begin19.preheader:                            ; preds = %for_end12
  %40 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 32
  %41 = bitcast float* %40 to <32 x float>*
  %42 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 64
  %43 = bitcast float* %42 to <32 x float>*
  %44 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 96
  %45 = bitcast float* %44 to <32 x float>*
  %46 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 128
  %47 = bitcast float* %46 to <32 x float>*
  %48 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 160
  %49 = bitcast float* %48 to <32 x float>*
  %50 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 192
  %51 = bitcast float* %50 to <32 x float>*
  %52 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 224
  %53 = bitcast float* %52 to <32 x float>*
  %54 = bitcast i8* %2 to float*
  %55 = bitcast [8 x <32 x float>]* %3 to i8*
  br label %for_begin22.preheader

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvars.iv86 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next87, %for_end15 ]
  %56 = mul nuw nsw i64 %indvars.iv86, 4608
  %57 = add nuw nsw i64 %56, %39
  %58 = mul nuw nsw i64 %indvars.iv86, 144
  %59 = add nuw nsw i64 %58, %38
  br label %for_begin16.preheader

for_end12:                                        ; preds = %for_end15
  %60 = add nuw nsw i32 %30, 1
  %exitcond89 = icmp eq i32 %60, 6
  br i1 %exitcond89, label %for_begin19.preheader, label %for_begin10.preheader, !prof !34

for_begin16.preheader:                            ; preds = %for_end18, %for_begin13.preheader
  %indvars.iv83 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next84, %for_end18 ]
  %61 = shl i64 %indvars.iv83, 9
  %62 = add nuw nsw i64 %57, %61
  %63 = add nuw nsw i64 %59, %indvars.iv83
  br label %for_body17

for_end15:                                        ; preds = %for_end18
  %indvars.iv.next87 = add nuw nsw i64 %indvars.iv86, 1
  %exitcond88 = icmp eq i64 %indvars.iv.next87, 4
  br i1 %exitcond88, label %for_end12, label %for_begin13.preheader, !prof !34

for_body17:                                       ; preds = %for_body17, %for_begin16.preheader
  %indvars.iv80 = phi i64 [ 0, %for_begin16.preheader ], [ %indvars.iv.next81, %for_body17 ]
  %64 = shl i64 %indvars.iv80, 5
  %65 = add nuw nsw i64 %62, %64
  %66 = mul nuw nsw i64 %indvars.iv80, 9
  %67 = add nuw nsw i64 %63, %66
  %68 = add nuw nsw i64 %67, 576
  %69 = add nuw nsw i64 %67, 1152
  %70 = add nuw nsw i64 %67, 1728
  %71 = add nuw nsw i64 %67, 2304
  %72 = add nuw nsw i64 %67, 2880
  %73 = add nuw nsw i64 %67, 3456
  %74 = add nuw nsw i64 %67, 4032
  %75 = add nuw nsw i64 %67, 4608
  %76 = add nuw nsw i64 %67, 5184
  %77 = add nuw nsw i64 %67, 5760
  %78 = add nuw nsw i64 %67, 6336
  %79 = add nuw nsw i64 %67, 6912
  %80 = add nuw nsw i64 %67, 7488
  %81 = add nuw nsw i64 %67, 8064
  %82 = add nuw nsw i64 %67, 8640
  %83 = add nuw nsw i64 %67, 9216
  %84 = add nuw nsw i64 %67, 9792
  %85 = add nuw nsw i64 %67, 10368
  %86 = add nuw nsw i64 %67, 10944
  %87 = add nuw nsw i64 %67, 11520
  %88 = add nuw nsw i64 %67, 12096
  %89 = add nuw nsw i64 %67, 12672
  %90 = add nuw nsw i64 %67, 13248
  %91 = add nuw nsw i64 %67, 13824
  %92 = add nuw nsw i64 %67, 14400
  %93 = add nuw nsw i64 %67, 14976
  %94 = add nuw nsw i64 %67, 15552
  %95 = add nuw nsw i64 %67, 16128
  %96 = add nuw nsw i64 %67, 16704
  %97 = add nuw nsw i64 %67, 17280
  %98 = add nuw nsw i64 %67, 17856
  %99 = getelementptr inbounds float, float* %29, i64 %67
  %100 = load float, float* %99, align 4, !tbaa !375
  %101 = insertelement <32 x float> undef, float %100, i32 0
  %102 = getelementptr inbounds float, float* %29, i64 %68
  %103 = load float, float* %102, align 4, !tbaa !375
  %104 = insertelement <32 x float> %101, float %103, i32 1
  %105 = getelementptr inbounds float, float* %29, i64 %69
  %106 = load float, float* %105, align 4, !tbaa !375
  %107 = insertelement <32 x float> %104, float %106, i32 2
  %108 = getelementptr inbounds float, float* %29, i64 %70
  %109 = load float, float* %108, align 4, !tbaa !375
  %110 = insertelement <32 x float> %107, float %109, i32 3
  %111 = getelementptr inbounds float, float* %29, i64 %71
  %112 = load float, float* %111, align 4, !tbaa !375
  %113 = insertelement <32 x float> %110, float %112, i32 4
  %114 = getelementptr inbounds float, float* %29, i64 %72
  %115 = load float, float* %114, align 4, !tbaa !375
  %116 = insertelement <32 x float> %113, float %115, i32 5
  %117 = getelementptr inbounds float, float* %29, i64 %73
  %118 = load float, float* %117, align 4, !tbaa !375
  %119 = insertelement <32 x float> %116, float %118, i32 6
  %120 = getelementptr inbounds float, float* %29, i64 %74
  %121 = load float, float* %120, align 4, !tbaa !375
  %122 = insertelement <32 x float> %119, float %121, i32 7
  %123 = getelementptr inbounds float, float* %29, i64 %75
  %124 = load float, float* %123, align 4, !tbaa !375
  %125 = insertelement <32 x float> %122, float %124, i32 8
  %126 = getelementptr inbounds float, float* %29, i64 %76
  %127 = load float, float* %126, align 4, !tbaa !375
  %128 = insertelement <32 x float> %125, float %127, i32 9
  %129 = getelementptr inbounds float, float* %29, i64 %77
  %130 = load float, float* %129, align 4, !tbaa !375
  %131 = insertelement <32 x float> %128, float %130, i32 10
  %132 = getelementptr inbounds float, float* %29, i64 %78
  %133 = load float, float* %132, align 4, !tbaa !375
  %134 = insertelement <32 x float> %131, float %133, i32 11
  %135 = getelementptr inbounds float, float* %29, i64 %79
  %136 = load float, float* %135, align 4, !tbaa !375
  %137 = insertelement <32 x float> %134, float %136, i32 12
  %138 = getelementptr inbounds float, float* %29, i64 %80
  %139 = load float, float* %138, align 4, !tbaa !375
  %140 = insertelement <32 x float> %137, float %139, i32 13
  %141 = getelementptr inbounds float, float* %29, i64 %81
  %142 = load float, float* %141, align 4, !tbaa !375
  %143 = insertelement <32 x float> %140, float %142, i32 14
  %144 = getelementptr inbounds float, float* %29, i64 %82
  %145 = load float, float* %144, align 4, !tbaa !375
  %146 = insertelement <32 x float> %143, float %145, i32 15
  %147 = getelementptr inbounds float, float* %29, i64 %83
  %148 = load float, float* %147, align 4, !tbaa !375
  %149 = insertelement <32 x float> %146, float %148, i32 16
  %150 = getelementptr inbounds float, float* %29, i64 %84
  %151 = load float, float* %150, align 4, !tbaa !375
  %152 = insertelement <32 x float> %149, float %151, i32 17
  %153 = getelementptr inbounds float, float* %29, i64 %85
  %154 = load float, float* %153, align 4, !tbaa !375
  %155 = insertelement <32 x float> %152, float %154, i32 18
  %156 = getelementptr inbounds float, float* %29, i64 %86
  %157 = load float, float* %156, align 4, !tbaa !375
  %158 = insertelement <32 x float> %155, float %157, i32 19
  %159 = getelementptr inbounds float, float* %29, i64 %87
  %160 = load float, float* %159, align 4, !tbaa !375
  %161 = insertelement <32 x float> %158, float %160, i32 20
  %162 = getelementptr inbounds float, float* %29, i64 %88
  %163 = load float, float* %162, align 4, !tbaa !375
  %164 = insertelement <32 x float> %161, float %163, i32 21
  %165 = getelementptr inbounds float, float* %29, i64 %89
  %166 = load float, float* %165, align 4, !tbaa !375
  %167 = insertelement <32 x float> %164, float %166, i32 22
  %168 = getelementptr inbounds float, float* %29, i64 %90
  %169 = load float, float* %168, align 4, !tbaa !375
  %170 = insertelement <32 x float> %167, float %169, i32 23
  %171 = getelementptr inbounds float, float* %29, i64 %91
  %172 = load float, float* %171, align 4, !tbaa !375
  %173 = insertelement <32 x float> %170, float %172, i32 24
  %174 = getelementptr inbounds float, float* %29, i64 %92
  %175 = load float, float* %174, align 4, !tbaa !375
  %176 = insertelement <32 x float> %173, float %175, i32 25
  %177 = getelementptr inbounds float, float* %29, i64 %93
  %178 = load float, float* %177, align 4, !tbaa !375
  %179 = insertelement <32 x float> %176, float %178, i32 26
  %180 = getelementptr inbounds float, float* %29, i64 %94
  %181 = load float, float* %180, align 4, !tbaa !375
  %182 = insertelement <32 x float> %179, float %181, i32 27
  %183 = getelementptr inbounds float, float* %29, i64 %95
  %184 = load float, float* %183, align 4, !tbaa !375
  %185 = insertelement <32 x float> %182, float %184, i32 28
  %186 = getelementptr inbounds float, float* %29, i64 %96
  %187 = load float, float* %186, align 4, !tbaa !375
  %188 = insertelement <32 x float> %185, float %187, i32 29
  %189 = getelementptr inbounds float, float* %29, i64 %97
  %190 = load float, float* %189, align 4, !tbaa !375
  %191 = insertelement <32 x float> %188, float %190, i32 30
  %192 = getelementptr inbounds float, float* %29, i64 %98
  %193 = load float, float* %192, align 4, !tbaa !375
  %194 = insertelement <32 x float> %191, float %193, i32 31
  %195 = getelementptr inbounds [1152 x <32 x float>], [1152 x <32 x float>]* %5, i64 0, i64 0, i64 %65
  %196 = bitcast float* %195 to <32 x float>*
  store <32 x float> %194, <32 x float>* %196, align 16, !tbaa !378
  %indvars.iv.next81 = add nuw nsw i64 %indvars.iv80, 1
  %exitcond82 = icmp eq i64 %indvars.iv.next81, 16
  br i1 %exitcond82, label %for_end18, label %for_body17, !prof !34

for_end18:                                        ; preds = %for_body17
  %indvars.iv.next84 = add nuw nsw i64 %indvars.iv83, 1
  %exitcond85 = icmp eq i64 %indvars.iv.next84, 3
  br i1 %exitcond85, label %for_end15, label %for_begin16.preheader, !prof !34

for_begin22.preheader:                            ; preds = %for_end39, %for_begin19.preheader
  %197 = phi i32 [ 0, %for_begin19.preheader ], [ %326, %for_end39 ]
  %198 = urem i32 %197, 224
  %199 = udiv i32 %197, 224
  %200 = mul nsw i32 %199, 18432
  %201 = zext i32 %200 to i64
  %reass.mul = mul nuw nsw i32 %198, 3616
  %202 = zext i32 %reass.mul to i64
  %203 = mul nuw nsw i32 %198, 3616
  %reass.mul.1 = add nuw nsw i32 %203, 3616
  %204 = zext i32 %reass.mul.1 to i64
  %205 = mul nuw nsw i32 %198, 3616
  %reass.mul.2 = add nuw nsw i32 %205, 7232
  %206 = zext i32 %reass.mul.2 to i64
  br label %for_body23

for_end21:                                        ; preds = %for_end39
  ret void

for_begin37.preheader:                            ; preds = %for_begin34.preheader
  %207 = mul nuw nsw i32 %198, 224
  %208 = mul nsw i32 %199, 1605632
  %209 = add nuw nsw i32 %208, %207
  %210 = zext i32 %209 to i64
  br label %for_begin40.preheader

for_body23:                                       ; preds = %for_begin34.preheader, %for_begin22.preheader
  %indvar = phi i64 [ 0, %for_begin22.preheader ], [ %indvar.next, %for_begin34.preheader ]
  %211 = shl i64 %indvar, 3
  %scevgep = getelementptr [224 x <32 x float>], [224 x <32 x float>]* %4, i64 0, i64 %211
  %scevgep71 = bitcast <32 x float>* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %55, i8 0, i64 1024, i1 false)
  br label %for_begin28.preheader

for_begin34.preheader:                            ; preds = %for_end33.2
  store <32 x float> %621, <32 x float>* %.sub, align 16, !tbaa !381
  store <32 x float> %622, <32 x float>* %41, align 16, !tbaa !381
  store <32 x float> %623, <32 x float>* %43, align 16, !tbaa !381
  store <32 x float> %624, <32 x float>* %45, align 16, !tbaa !381
  store <32 x float> %625, <32 x float>* %47, align 16, !tbaa !381
  store <32 x float> %626, <32 x float>* %49, align 16, !tbaa !381
  store <32 x float> %627, <32 x float>* %51, align 16, !tbaa !381
  store <32 x float> %633, <32 x float>* %53, align 16, !tbaa !381
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep71, i8* nonnull align 16 %28, i64 1024, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond72 = icmp eq i64 %indvar.next, 28
  br i1 %exitcond72, label %for_begin37.preheader, label %for_body23, !prof !34

for_begin28.preheader:                            ; preds = %for_end33.2, %for_body23
  %indvars.iv65 = phi i64 [ 0, %for_body23 ], [ %indvars.iv.next66, %for_end33.2 ]
  %.lcssa16.lcssa45 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %633, %for_end33.2 ]
  %.lcssa14.lcssa43 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %627, %for_end33.2 ]
  %.lcssa12.lcssa41 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %626, %for_end33.2 ]
  %.lcssa10.lcssa39 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %625, %for_end33.2 ]
  %.lcssa8.lcssa37 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %624, %for_end33.2 ]
  %.lcssa6.lcssa36 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %623, %for_end33.2 ]
  %.lcssa4.lcssa34 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %622, %for_end33.2 ]
  %.lcssa.lcssa32 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %621, %for_end33.2 ]
  %212 = mul nuw nsw i64 %indvars.iv65, 817216
  %213 = add nuw nsw i64 %212, %211
  %214 = mul nuw nsw i64 %indvars.iv65, 4608
  %215 = add nuw nsw i64 %214, %201
  %216 = add nsw i64 %213, %202
  %217 = trunc i64 %216 to i32
  br label %for_body32

for_body32:                                       ; preds = %for_body32, %for_begin28.preheader
  %indvars.iv = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next, %for_body32 ]
  %218 = phi <32 x float> [ %.lcssa16.lcssa45, %for_begin28.preheader ], [ %318, %for_body32 ]
  %219 = phi <32 x float> [ %.lcssa14.lcssa43, %for_begin28.preheader ], [ %312, %for_body32 ]
  %220 = phi <32 x float> [ %.lcssa12.lcssa41, %for_begin28.preheader ], [ %311, %for_body32 ]
  %221 = phi <32 x float> [ %.lcssa10.lcssa39, %for_begin28.preheader ], [ %310, %for_body32 ]
  %222 = phi <32 x float> [ %.lcssa8.lcssa37, %for_begin28.preheader ], [ %309, %for_body32 ]
  %223 = phi <32 x float> [ %.lcssa6.lcssa36, %for_begin28.preheader ], [ %308, %for_body32 ]
  %224 = phi <32 x float> [ %.lcssa4.lcssa34, %for_begin28.preheader ], [ %307, %for_body32 ]
  %225 = phi <32 x float> [ %.lcssa.lcssa32, %for_begin28.preheader ], [ %306, %for_body32 ]
  %226 = phi i32 [ 0, %for_begin28.preheader ], [ %319, %for_body32 ]
  %227 = mul nuw nsw i64 %indvars.iv, 226
  %228 = mul nuw nsw i32 %226, 226
  %229 = add nsw i64 %216, %227
  %230 = add nsw i32 %228, %217
  %231 = and i64 %229, 4294967294
  %232 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %231
  %233 = load float, float* %232, align 8, !tbaa !372
  %234 = insertelement <32 x float> undef, float %233, i32 0
  %235 = shufflevector <32 x float> %234, <32 x float> undef, <32 x i32> zeroinitializer
  %236 = shl nsw i64 %indvars.iv, 5
  %237 = add nuw nsw i64 %215, %236
  %238 = getelementptr inbounds [1152 x <32 x float>], [1152 x <32 x float>]* %5, i64 0, i64 0, i64 %237
  %239 = bitcast float* %238 to <32 x float>*
  %240 = load <32 x float>, <32 x float>* %239, align 16, !tbaa !378
  %241 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %235, <32 x float> %240, <32 x float> %225)
  %242 = or i32 %230, 1
  %243 = sext i32 %242 to i64
  %244 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %243
  %245 = load float, float* %244, align 4, !tbaa !372
  %246 = insertelement <32 x float> undef, float %245, i32 0
  %247 = shufflevector <32 x float> %246, <32 x float> undef, <32 x i32> zeroinitializer
  %248 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %247, <32 x float> %240, <32 x float> %224)
  %249 = add nuw nsw i64 %229, 2
  %250 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %249
  %251 = load float, float* %250, align 8, !tbaa !372
  %252 = insertelement <32 x float> undef, float %251, i32 0
  %253 = shufflevector <32 x float> %252, <32 x float> undef, <32 x i32> zeroinitializer
  %254 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %253, <32 x float> %240, <32 x float> %223)
  %255 = add nuw nsw i64 %229, 3
  %256 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %255
  %257 = load float, float* %256, align 4, !tbaa !372
  %258 = insertelement <32 x float> undef, float %257, i32 0
  %259 = shufflevector <32 x float> %258, <32 x float> undef, <32 x i32> zeroinitializer
  %260 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %259, <32 x float> %240, <32 x float> %222)
  %261 = add nuw nsw i64 %229, 4
  %262 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %261
  %263 = load float, float* %262, align 8, !tbaa !372
  %264 = insertelement <32 x float> undef, float %263, i32 0
  %265 = shufflevector <32 x float> %264, <32 x float> undef, <32 x i32> zeroinitializer
  %266 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %265, <32 x float> %240, <32 x float> %221)
  %267 = add nuw nsw i64 %229, 5
  %268 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %267
  %269 = load float, float* %268, align 4, !tbaa !372
  %270 = insertelement <32 x float> undef, float %269, i32 0
  %271 = shufflevector <32 x float> %270, <32 x float> undef, <32 x i32> zeroinitializer
  %272 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %271, <32 x float> %240, <32 x float> %220)
  %273 = add nuw nsw i64 %229, 6
  %274 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %273
  %275 = load float, float* %274, align 8, !tbaa !372
  %276 = insertelement <32 x float> undef, float %275, i32 0
  %277 = shufflevector <32 x float> %276, <32 x float> undef, <32 x i32> zeroinitializer
  %278 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %277, <32 x float> %240, <32 x float> %219)
  %279 = add nuw nsw i64 %229, 7
  %280 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %279
  %281 = load float, float* %280, align 4, !tbaa !372
  %282 = insertelement <32 x float> undef, float %281, i32 0
  %283 = shufflevector <32 x float> %282, <32 x float> undef, <32 x i32> zeroinitializer
  %284 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %283, <32 x float> %240, <32 x float> %218)
  %285 = add nuw nsw i64 %237, 512
  %286 = getelementptr inbounds [1152 x <32 x float>], [1152 x <32 x float>]* %5, i64 0, i64 0, i64 %285
  %287 = bitcast float* %286 to <32 x float>*
  %288 = load <32 x float>, <32 x float>* %287, align 16, !tbaa !378
  %289 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %247, <32 x float> %288, <32 x float> %241)
  %290 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %253, <32 x float> %288, <32 x float> %248)
  %291 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %259, <32 x float> %288, <32 x float> %254)
  %292 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %265, <32 x float> %288, <32 x float> %260)
  %293 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %271, <32 x float> %288, <32 x float> %266)
  %294 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %277, <32 x float> %288, <32 x float> %272)
  %295 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %283, <32 x float> %288, <32 x float> %278)
  %296 = add nuw nsw i64 %229, 8
  %297 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %296
  %298 = load float, float* %297, align 8, !tbaa !372
  %299 = insertelement <32 x float> undef, float %298, i32 0
  %300 = shufflevector <32 x float> %299, <32 x float> undef, <32 x i32> zeroinitializer
  %301 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %300, <32 x float> %288, <32 x float> %284)
  %302 = add nuw nsw i64 %237, 1024
  %303 = getelementptr inbounds [1152 x <32 x float>], [1152 x <32 x float>]* %5, i64 0, i64 0, i64 %302
  %304 = bitcast float* %303 to <32 x float>*
  %305 = load <32 x float>, <32 x float>* %304, align 16, !tbaa !378
  %306 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %253, <32 x float> %305, <32 x float> %289)
  %307 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %259, <32 x float> %305, <32 x float> %290)
  %308 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %265, <32 x float> %305, <32 x float> %291)
  %309 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %271, <32 x float> %305, <32 x float> %292)
  %310 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %277, <32 x float> %305, <32 x float> %293)
  %311 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %283, <32 x float> %305, <32 x float> %294)
  %312 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %300, <32 x float> %305, <32 x float> %295)
  %313 = add nuw nsw i64 %229, 9
  %314 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %313
  %315 = load float, float* %314, align 4, !tbaa !372
  %316 = insertelement <32 x float> undef, float %315, i32 0
  %317 = shufflevector <32 x float> %316, <32 x float> undef, <32 x i32> zeroinitializer
  %318 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %317, <32 x float> %305, <32 x float> %301)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %319 = add nuw nsw i32 %226, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 16
  br i1 %exitcond, label %for_end33, label %for_body32, !prof !34

for_end33:                                        ; preds = %for_body32
  %320 = add nsw i64 %213, %204
  %321 = add nuw nsw i64 %215, 1536
  %322 = trunc i64 %320 to i32
  br label %for_body32.1

for_begin40.preheader:                            ; preds = %for_end42, %for_begin37.preheader
  %indvars.iv76 = phi i64 [ 0, %for_begin37.preheader ], [ %indvars.iv.next77, %for_end42 ]
  %323 = shl i64 %indvars.iv76, 3
  %324 = add nuw nsw i64 %323, %210
  %325 = shl i64 %indvars.iv76, 8
  br label %for_body41

for_end39:                                        ; preds = %for_end42
  %326 = add nuw nsw i32 %197, 1
  %exitcond79 = icmp eq i32 %326, 448
  br i1 %exitcond79, label %for_end21, label %for_begin22.preheader, !prof !34

for_body41:                                       ; preds = %for_body41, %for_begin40.preheader
  %indvars.iv73 = phi i64 [ 0, %for_begin40.preheader ], [ %indvars.iv.next74, %for_body41 ]
  %327 = add nuw nsw i64 %324, %indvars.iv73
  %328 = add nuw nsw i64 %327, 50176
  %329 = add nuw nsw i64 %327, 100352
  %330 = add nuw nsw i64 %327, 150528
  %331 = add nuw nsw i64 %327, 200704
  %332 = add nuw nsw i64 %327, 250880
  %333 = add nuw nsw i64 %327, 301056
  %334 = add nuw nsw i64 %327, 351232
  %335 = add nuw nsw i64 %327, 401408
  %336 = add nuw nsw i64 %327, 451584
  %337 = add nuw nsw i64 %327, 501760
  %338 = add nuw nsw i64 %327, 551936
  %339 = add nuw nsw i64 %327, 602112
  %340 = add nuw nsw i64 %327, 652288
  %341 = add nuw nsw i64 %327, 702464
  %342 = add nuw nsw i64 %327, 752640
  %343 = add nuw nsw i64 %327, 802816
  %344 = add nuw nsw i64 %327, 852992
  %345 = add nuw nsw i64 %327, 903168
  %346 = add nuw nsw i64 %327, 953344
  %347 = add nuw nsw i64 %327, 1003520
  %348 = add nuw nsw i64 %327, 1053696
  %349 = add nuw nsw i64 %327, 1103872
  %350 = add nuw nsw i64 %327, 1154048
  %351 = add nuw nsw i64 %327, 1204224
  %352 = add nuw nsw i64 %327, 1254400
  %353 = add nuw nsw i64 %327, 1304576
  %354 = add nuw nsw i64 %327, 1354752
  %355 = add nuw nsw i64 %327, 1404928
  %356 = add nuw nsw i64 %327, 1455104
  %357 = add nuw nsw i64 %327, 1505280
  %358 = add nuw nsw i64 %327, 1555456
  %359 = shl i64 %indvars.iv73, 5
  %360 = add nuw nsw i64 %359, %325
  %361 = getelementptr inbounds [224 x <32 x float>], [224 x <32 x float>]* %4, i64 0, i64 0, i64 %360
  %362 = bitcast float* %361 to <32 x float>*
  %363 = load <32 x float>, <32 x float>* %362, align 16, !tbaa !390
  %364 = getelementptr inbounds float, float* %54, i64 %327
  %365 = extractelement <32 x float> %363, i64 0
  store float %365, float* %364, align 4, !tbaa !393
  %366 = getelementptr inbounds float, float* %54, i64 %328
  %367 = extractelement <32 x float> %363, i64 1
  store float %367, float* %366, align 4, !tbaa !393
  %368 = getelementptr inbounds float, float* %54, i64 %329
  %369 = extractelement <32 x float> %363, i64 2
  store float %369, float* %368, align 4, !tbaa !393
  %370 = getelementptr inbounds float, float* %54, i64 %330
  %371 = extractelement <32 x float> %363, i64 3
  store float %371, float* %370, align 4, !tbaa !393
  %372 = getelementptr inbounds float, float* %54, i64 %331
  %373 = extractelement <32 x float> %363, i64 4
  store float %373, float* %372, align 4, !tbaa !393
  %374 = getelementptr inbounds float, float* %54, i64 %332
  %375 = extractelement <32 x float> %363, i64 5
  store float %375, float* %374, align 4, !tbaa !393
  %376 = getelementptr inbounds float, float* %54, i64 %333
  %377 = extractelement <32 x float> %363, i64 6
  store float %377, float* %376, align 4, !tbaa !393
  %378 = getelementptr inbounds float, float* %54, i64 %334
  %379 = extractelement <32 x float> %363, i64 7
  store float %379, float* %378, align 4, !tbaa !393
  %380 = getelementptr inbounds float, float* %54, i64 %335
  %381 = extractelement <32 x float> %363, i64 8
  store float %381, float* %380, align 4, !tbaa !393
  %382 = getelementptr inbounds float, float* %54, i64 %336
  %383 = extractelement <32 x float> %363, i64 9
  store float %383, float* %382, align 4, !tbaa !393
  %384 = getelementptr inbounds float, float* %54, i64 %337
  %385 = extractelement <32 x float> %363, i64 10
  store float %385, float* %384, align 4, !tbaa !393
  %386 = getelementptr inbounds float, float* %54, i64 %338
  %387 = extractelement <32 x float> %363, i64 11
  store float %387, float* %386, align 4, !tbaa !393
  %388 = getelementptr inbounds float, float* %54, i64 %339
  %389 = extractelement <32 x float> %363, i64 12
  store float %389, float* %388, align 4, !tbaa !393
  %390 = getelementptr inbounds float, float* %54, i64 %340
  %391 = extractelement <32 x float> %363, i64 13
  store float %391, float* %390, align 4, !tbaa !393
  %392 = getelementptr inbounds float, float* %54, i64 %341
  %393 = extractelement <32 x float> %363, i64 14
  store float %393, float* %392, align 4, !tbaa !393
  %394 = getelementptr inbounds float, float* %54, i64 %342
  %395 = extractelement <32 x float> %363, i64 15
  store float %395, float* %394, align 4, !tbaa !393
  %396 = getelementptr inbounds float, float* %54, i64 %343
  %397 = extractelement <32 x float> %363, i64 16
  store float %397, float* %396, align 4, !tbaa !393
  %398 = getelementptr inbounds float, float* %54, i64 %344
  %399 = extractelement <32 x float> %363, i64 17
  store float %399, float* %398, align 4, !tbaa !393
  %400 = getelementptr inbounds float, float* %54, i64 %345
  %401 = extractelement <32 x float> %363, i64 18
  store float %401, float* %400, align 4, !tbaa !393
  %402 = getelementptr inbounds float, float* %54, i64 %346
  %403 = extractelement <32 x float> %363, i64 19
  store float %403, float* %402, align 4, !tbaa !393
  %404 = getelementptr inbounds float, float* %54, i64 %347
  %405 = extractelement <32 x float> %363, i64 20
  store float %405, float* %404, align 4, !tbaa !393
  %406 = getelementptr inbounds float, float* %54, i64 %348
  %407 = extractelement <32 x float> %363, i64 21
  store float %407, float* %406, align 4, !tbaa !393
  %408 = getelementptr inbounds float, float* %54, i64 %349
  %409 = extractelement <32 x float> %363, i64 22
  store float %409, float* %408, align 4, !tbaa !393
  %410 = getelementptr inbounds float, float* %54, i64 %350
  %411 = extractelement <32 x float> %363, i64 23
  store float %411, float* %410, align 4, !tbaa !393
  %412 = getelementptr inbounds float, float* %54, i64 %351
  %413 = extractelement <32 x float> %363, i64 24
  store float %413, float* %412, align 4, !tbaa !393
  %414 = getelementptr inbounds float, float* %54, i64 %352
  %415 = extractelement <32 x float> %363, i64 25
  store float %415, float* %414, align 4, !tbaa !393
  %416 = getelementptr inbounds float, float* %54, i64 %353
  %417 = extractelement <32 x float> %363, i64 26
  store float %417, float* %416, align 4, !tbaa !393
  %418 = getelementptr inbounds float, float* %54, i64 %354
  %419 = extractelement <32 x float> %363, i64 27
  store float %419, float* %418, align 4, !tbaa !393
  %420 = getelementptr inbounds float, float* %54, i64 %355
  %421 = extractelement <32 x float> %363, i64 28
  store float %421, float* %420, align 4, !tbaa !393
  %422 = getelementptr inbounds float, float* %54, i64 %356
  %423 = extractelement <32 x float> %363, i64 29
  store float %423, float* %422, align 4, !tbaa !393
  %424 = getelementptr inbounds float, float* %54, i64 %357
  %425 = extractelement <32 x float> %363, i64 30
  store float %425, float* %424, align 4, !tbaa !393
  %426 = getelementptr inbounds float, float* %54, i64 %358
  %427 = extractelement <32 x float> %363, i64 31
  store float %427, float* %426, align 4, !tbaa !393
  %indvars.iv.next74 = add nuw nsw i64 %indvars.iv73, 1
  %exitcond75 = icmp eq i64 %indvars.iv.next74, 8
  br i1 %exitcond75, label %for_end42, label %for_body41, !prof !34

for_end42:                                        ; preds = %for_body41
  %indvars.iv.next77 = add nuw nsw i64 %indvars.iv76, 1
  %exitcond78 = icmp eq i64 %indvars.iv.next77, 28
  br i1 %exitcond78, label %for_end39, label %for_begin40.preheader, !prof !34

for_body32.1:                                     ; preds = %for_body32.1, %for_end33
  %indvars.iv.1 = phi i64 [ 0, %for_end33 ], [ %indvars.iv.next.1, %for_body32.1 ]
  %428 = phi <32 x float> [ %318, %for_end33 ], [ %528, %for_body32.1 ]
  %429 = phi <32 x float> [ %312, %for_end33 ], [ %522, %for_body32.1 ]
  %430 = phi <32 x float> [ %311, %for_end33 ], [ %521, %for_body32.1 ]
  %431 = phi <32 x float> [ %310, %for_end33 ], [ %520, %for_body32.1 ]
  %432 = phi <32 x float> [ %309, %for_end33 ], [ %519, %for_body32.1 ]
  %433 = phi <32 x float> [ %308, %for_end33 ], [ %518, %for_body32.1 ]
  %434 = phi <32 x float> [ %307, %for_end33 ], [ %517, %for_body32.1 ]
  %435 = phi <32 x float> [ %306, %for_end33 ], [ %516, %for_body32.1 ]
  %436 = phi i32 [ 0, %for_end33 ], [ %529, %for_body32.1 ]
  %437 = mul nuw nsw i64 %indvars.iv.1, 226
  %438 = mul nuw nsw i32 %436, 226
  %439 = add nsw i64 %320, %437
  %440 = add nsw i32 %438, %322
  %441 = and i64 %439, 4294967294
  %442 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %441
  %443 = load float, float* %442, align 8, !tbaa !372
  %444 = insertelement <32 x float> undef, float %443, i32 0
  %445 = shufflevector <32 x float> %444, <32 x float> undef, <32 x i32> zeroinitializer
  %446 = shl nsw i64 %indvars.iv.1, 5
  %447 = add nuw nsw i64 %321, %446
  %448 = getelementptr inbounds [1152 x <32 x float>], [1152 x <32 x float>]* %5, i64 0, i64 0, i64 %447
  %449 = bitcast float* %448 to <32 x float>*
  %450 = load <32 x float>, <32 x float>* %449, align 16, !tbaa !378
  %451 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %445, <32 x float> %450, <32 x float> %435)
  %452 = or i32 %440, 1
  %453 = sext i32 %452 to i64
  %454 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %453
  %455 = load float, float* %454, align 4, !tbaa !372
  %456 = insertelement <32 x float> undef, float %455, i32 0
  %457 = shufflevector <32 x float> %456, <32 x float> undef, <32 x i32> zeroinitializer
  %458 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %457, <32 x float> %450, <32 x float> %434)
  %459 = add nuw nsw i64 %439, 2
  %460 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %459
  %461 = load float, float* %460, align 8, !tbaa !372
  %462 = insertelement <32 x float> undef, float %461, i32 0
  %463 = shufflevector <32 x float> %462, <32 x float> undef, <32 x i32> zeroinitializer
  %464 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %463, <32 x float> %450, <32 x float> %433)
  %465 = add nuw nsw i64 %439, 3
  %466 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %465
  %467 = load float, float* %466, align 4, !tbaa !372
  %468 = insertelement <32 x float> undef, float %467, i32 0
  %469 = shufflevector <32 x float> %468, <32 x float> undef, <32 x i32> zeroinitializer
  %470 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %469, <32 x float> %450, <32 x float> %432)
  %471 = add nuw nsw i64 %439, 4
  %472 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %471
  %473 = load float, float* %472, align 8, !tbaa !372
  %474 = insertelement <32 x float> undef, float %473, i32 0
  %475 = shufflevector <32 x float> %474, <32 x float> undef, <32 x i32> zeroinitializer
  %476 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %475, <32 x float> %450, <32 x float> %431)
  %477 = add nuw nsw i64 %439, 5
  %478 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %477
  %479 = load float, float* %478, align 4, !tbaa !372
  %480 = insertelement <32 x float> undef, float %479, i32 0
  %481 = shufflevector <32 x float> %480, <32 x float> undef, <32 x i32> zeroinitializer
  %482 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %481, <32 x float> %450, <32 x float> %430)
  %483 = add nuw nsw i64 %439, 6
  %484 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %483
  %485 = load float, float* %484, align 8, !tbaa !372
  %486 = insertelement <32 x float> undef, float %485, i32 0
  %487 = shufflevector <32 x float> %486, <32 x float> undef, <32 x i32> zeroinitializer
  %488 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %487, <32 x float> %450, <32 x float> %429)
  %489 = add nuw nsw i64 %439, 7
  %490 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %489
  %491 = load float, float* %490, align 4, !tbaa !372
  %492 = insertelement <32 x float> undef, float %491, i32 0
  %493 = shufflevector <32 x float> %492, <32 x float> undef, <32 x i32> zeroinitializer
  %494 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %493, <32 x float> %450, <32 x float> %428)
  %495 = add nuw nsw i64 %447, 512
  %496 = getelementptr inbounds [1152 x <32 x float>], [1152 x <32 x float>]* %5, i64 0, i64 0, i64 %495
  %497 = bitcast float* %496 to <32 x float>*
  %498 = load <32 x float>, <32 x float>* %497, align 16, !tbaa !378
  %499 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %457, <32 x float> %498, <32 x float> %451)
  %500 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %463, <32 x float> %498, <32 x float> %458)
  %501 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %469, <32 x float> %498, <32 x float> %464)
  %502 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %475, <32 x float> %498, <32 x float> %470)
  %503 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %481, <32 x float> %498, <32 x float> %476)
  %504 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %487, <32 x float> %498, <32 x float> %482)
  %505 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %493, <32 x float> %498, <32 x float> %488)
  %506 = add nuw nsw i64 %439, 8
  %507 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %506
  %508 = load float, float* %507, align 8, !tbaa !372
  %509 = insertelement <32 x float> undef, float %508, i32 0
  %510 = shufflevector <32 x float> %509, <32 x float> undef, <32 x i32> zeroinitializer
  %511 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %510, <32 x float> %498, <32 x float> %494)
  %512 = add nuw nsw i64 %447, 1024
  %513 = getelementptr inbounds [1152 x <32 x float>], [1152 x <32 x float>]* %5, i64 0, i64 0, i64 %512
  %514 = bitcast float* %513 to <32 x float>*
  %515 = load <32 x float>, <32 x float>* %514, align 16, !tbaa !378
  %516 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %463, <32 x float> %515, <32 x float> %499)
  %517 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %469, <32 x float> %515, <32 x float> %500)
  %518 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %475, <32 x float> %515, <32 x float> %501)
  %519 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %481, <32 x float> %515, <32 x float> %502)
  %520 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %487, <32 x float> %515, <32 x float> %503)
  %521 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %493, <32 x float> %515, <32 x float> %504)
  %522 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %510, <32 x float> %515, <32 x float> %505)
  %523 = add nuw nsw i64 %439, 9
  %524 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %523
  %525 = load float, float* %524, align 4, !tbaa !372
  %526 = insertelement <32 x float> undef, float %525, i32 0
  %527 = shufflevector <32 x float> %526, <32 x float> undef, <32 x i32> zeroinitializer
  %528 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %527, <32 x float> %515, <32 x float> %511)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %529 = add nuw nsw i32 %436, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 16
  br i1 %exitcond.1, label %for_end33.1, label %for_body32.1, !prof !34

for_end33.1:                                      ; preds = %for_body32.1
  %530 = add nsw i64 %213, %206
  %531 = add nuw nsw i64 %215, 3072
  %532 = trunc i64 %530 to i32
  br label %for_body32.2

for_body32.2:                                     ; preds = %for_body32.2, %for_end33.1
  %indvars.iv.2 = phi i64 [ 0, %for_end33.1 ], [ %indvars.iv.next.2, %for_body32.2 ]
  %533 = phi <32 x float> [ %528, %for_end33.1 ], [ %633, %for_body32.2 ]
  %534 = phi <32 x float> [ %522, %for_end33.1 ], [ %627, %for_body32.2 ]
  %535 = phi <32 x float> [ %521, %for_end33.1 ], [ %626, %for_body32.2 ]
  %536 = phi <32 x float> [ %520, %for_end33.1 ], [ %625, %for_body32.2 ]
  %537 = phi <32 x float> [ %519, %for_end33.1 ], [ %624, %for_body32.2 ]
  %538 = phi <32 x float> [ %518, %for_end33.1 ], [ %623, %for_body32.2 ]
  %539 = phi <32 x float> [ %517, %for_end33.1 ], [ %622, %for_body32.2 ]
  %540 = phi <32 x float> [ %516, %for_end33.1 ], [ %621, %for_body32.2 ]
  %541 = phi i32 [ 0, %for_end33.1 ], [ %634, %for_body32.2 ]
  %542 = mul nuw nsw i64 %indvars.iv.2, 226
  %543 = mul nuw nsw i32 %541, 226
  %544 = add nsw i64 %530, %542
  %545 = add nsw i32 %543, %532
  %546 = and i64 %544, 4294967294
  %547 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %546
  %548 = load float, float* %547, align 8, !tbaa !372
  %549 = insertelement <32 x float> undef, float %548, i32 0
  %550 = shufflevector <32 x float> %549, <32 x float> undef, <32 x i32> zeroinitializer
  %551 = shl nsw i64 %indvars.iv.2, 5
  %552 = add nuw nsw i64 %531, %551
  %553 = getelementptr inbounds [1152 x <32 x float>], [1152 x <32 x float>]* %5, i64 0, i64 0, i64 %552
  %554 = bitcast float* %553 to <32 x float>*
  %555 = load <32 x float>, <32 x float>* %554, align 16, !tbaa !378
  %556 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %550, <32 x float> %555, <32 x float> %540)
  %557 = or i32 %545, 1
  %558 = sext i32 %557 to i64
  %559 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %558
  %560 = load float, float* %559, align 4, !tbaa !372
  %561 = insertelement <32 x float> undef, float %560, i32 0
  %562 = shufflevector <32 x float> %561, <32 x float> undef, <32 x i32> zeroinitializer
  %563 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %562, <32 x float> %555, <32 x float> %539)
  %564 = add nuw nsw i64 %544, 2
  %565 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %564
  %566 = load float, float* %565, align 8, !tbaa !372
  %567 = insertelement <32 x float> undef, float %566, i32 0
  %568 = shufflevector <32 x float> %567, <32 x float> undef, <32 x i32> zeroinitializer
  %569 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %568, <32 x float> %555, <32 x float> %538)
  %570 = add nuw nsw i64 %544, 3
  %571 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %570
  %572 = load float, float* %571, align 4, !tbaa !372
  %573 = insertelement <32 x float> undef, float %572, i32 0
  %574 = shufflevector <32 x float> %573, <32 x float> undef, <32 x i32> zeroinitializer
  %575 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %574, <32 x float> %555, <32 x float> %537)
  %576 = add nuw nsw i64 %544, 4
  %577 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %576
  %578 = load float, float* %577, align 8, !tbaa !372
  %579 = insertelement <32 x float> undef, float %578, i32 0
  %580 = shufflevector <32 x float> %579, <32 x float> undef, <32 x i32> zeroinitializer
  %581 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %580, <32 x float> %555, <32 x float> %536)
  %582 = add nuw nsw i64 %544, 5
  %583 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %582
  %584 = load float, float* %583, align 4, !tbaa !372
  %585 = insertelement <32 x float> undef, float %584, i32 0
  %586 = shufflevector <32 x float> %585, <32 x float> undef, <32 x i32> zeroinitializer
  %587 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %586, <32 x float> %555, <32 x float> %535)
  %588 = add nuw nsw i64 %544, 6
  %589 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %588
  %590 = load float, float* %589, align 8, !tbaa !372
  %591 = insertelement <32 x float> undef, float %590, i32 0
  %592 = shufflevector <32 x float> %591, <32 x float> undef, <32 x i32> zeroinitializer
  %593 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %592, <32 x float> %555, <32 x float> %534)
  %594 = add nuw nsw i64 %544, 7
  %595 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %594
  %596 = load float, float* %595, align 4, !tbaa !372
  %597 = insertelement <32 x float> undef, float %596, i32 0
  %598 = shufflevector <32 x float> %597, <32 x float> undef, <32 x i32> zeroinitializer
  %599 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %598, <32 x float> %555, <32 x float> %533)
  %600 = add nuw nsw i64 %552, 512
  %601 = getelementptr inbounds [1152 x <32 x float>], [1152 x <32 x float>]* %5, i64 0, i64 0, i64 %600
  %602 = bitcast float* %601 to <32 x float>*
  %603 = load <32 x float>, <32 x float>* %602, align 16, !tbaa !378
  %604 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %562, <32 x float> %603, <32 x float> %556)
  %605 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %568, <32 x float> %603, <32 x float> %563)
  %606 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %574, <32 x float> %603, <32 x float> %569)
  %607 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %580, <32 x float> %603, <32 x float> %575)
  %608 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %586, <32 x float> %603, <32 x float> %581)
  %609 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %592, <32 x float> %603, <32 x float> %587)
  %610 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %598, <32 x float> %603, <32 x float> %593)
  %611 = add nuw nsw i64 %544, 8
  %612 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %611
  %613 = load float, float* %612, align 8, !tbaa !372
  %614 = insertelement <32 x float> undef, float %613, i32 0
  %615 = shufflevector <32 x float> %614, <32 x float> undef, <32 x i32> zeroinitializer
  %616 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %615, <32 x float> %603, <32 x float> %599)
  %617 = add nuw nsw i64 %552, 1024
  %618 = getelementptr inbounds [1152 x <32 x float>], [1152 x <32 x float>]* %5, i64 0, i64 0, i64 %617
  %619 = bitcast float* %618 to <32 x float>*
  %620 = load <32 x float>, <32 x float>* %619, align 16, !tbaa !378
  %621 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %568, <32 x float> %620, <32 x float> %604)
  %622 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %574, <32 x float> %620, <32 x float> %605)
  %623 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %580, <32 x float> %620, <32 x float> %606)
  %624 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %586, <32 x float> %620, <32 x float> %607)
  %625 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %592, <32 x float> %620, <32 x float> %608)
  %626 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %598, <32 x float> %620, <32 x float> %609)
  %627 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %615, <32 x float> %620, <32 x float> %610)
  %628 = add nuw nsw i64 %544, 9
  %629 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %628
  %630 = load float, float* %629, align 4, !tbaa !372
  %631 = insertelement <32 x float> undef, float %630, i32 0
  %632 = shufflevector <32 x float> %631, <32 x float> undef, <32 x i32> zeroinitializer
  %633 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %632, <32 x float> %620, <32 x float> %616)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %634 = add nuw nsw i32 %541, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 16
  br i1 %exitcond.2, label %for_end33.2, label %for_body32.2, !prof !34

for_end33.2:                                      ; preds = %for_body32.2
  %indvars.iv.next66 = add nuw nsw i64 %indvars.iv65, 1
  %exitcond67 = icmp eq i64 %indvars.iv.next66, 4
  br i1 %exitcond67, label %for_begin34.preheader, label %for_begin28.preheader, !prof !34

for_body5.us.us.1:                                ; preds = %if_end.us.us.1, %for_end6.us-lcssa.us.us
  %indvars.iv102.1 = phi i64 [ 0, %for_end6.us-lcssa.us.us ], [ %indvars.iv.next103.1, %if_end.us.us.1 ]
  %635 = phi i32 [ 0, %for_end6.us-lcssa.us.us ], [ %642, %if_end.us.us.1 ]
  %636 = add nuw nsw i64 %26, %indvars.iv102.1
  %trunc.us.us.1 = trunc i32 %635 to i31
  switch i31 %trunc.us.us.1, label %if_then.us.us.1 [
    i31 225, label %if_end.us.us.1
    i31 0, label %if_end.us.us.1
  ]

if_then.us.us.1:                                  ; preds = %for_body5.us.us.1
  %637 = add nsw i64 %27, %indvars.iv102.1
  %638 = getelementptr inbounds float, float* %7, i64 %637
  %639 = load float, float* %638, align 4, !tbaa !369
  br label %if_end.us.us.1

if_end.us.us.1:                                   ; preds = %if_then.us.us.1, %for_body5.us.us.1, %for_body5.us.us.1
  %640 = phi float [ %639, %if_then.us.us.1 ], [ 0.000000e+00, %for_body5.us.us.1 ], [ 0.000000e+00, %for_body5.us.us.1 ]
  %641 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %636
  store float %640, float* %641, align 4, !tbaa !372
  %indvars.iv.next103.1 = add nuw nsw i64 %indvars.iv102.1, 1
  %642 = add nuw nsw i32 %635, 1
  %exitcond104.1 = icmp eq i64 %indvars.iv.next103.1, 226
  br i1 %exitcond104.1, label %for_end6.us-lcssa.us.us.1, label %for_body5.us.us.1, !prof !34

for_end6.us-lcssa.us.us.1:                        ; preds = %if_end.us.us.1
  %643 = add nuw nsw i64 %8, 452
  %644 = add nsw i64 %17, 100352
  br label %for_body5.us.us.2

for_body5.us.us.2:                                ; preds = %if_end.us.us.2, %for_end6.us-lcssa.us.us.1
  %indvars.iv102.2 = phi i64 [ 0, %for_end6.us-lcssa.us.us.1 ], [ %indvars.iv.next103.2, %if_end.us.us.2 ]
  %645 = phi i32 [ 0, %for_end6.us-lcssa.us.us.1 ], [ %652, %if_end.us.us.2 ]
  %646 = add nuw nsw i64 %643, %indvars.iv102.2
  %trunc.us.us.2 = trunc i32 %645 to i31
  switch i31 %trunc.us.us.2, label %if_then.us.us.2 [
    i31 225, label %if_end.us.us.2
    i31 0, label %if_end.us.us.2
  ]

if_then.us.us.2:                                  ; preds = %for_body5.us.us.2
  %647 = add nsw i64 %644, %indvars.iv102.2
  %648 = getelementptr inbounds float, float* %7, i64 %647
  %649 = load float, float* %648, align 4, !tbaa !369
  br label %if_end.us.us.2

if_end.us.us.2:                                   ; preds = %if_then.us.us.2, %for_body5.us.us.2, %for_body5.us.us.2
  %650 = phi float [ %649, %if_then.us.us.2 ], [ 0.000000e+00, %for_body5.us.us.2 ], [ 0.000000e+00, %for_body5.us.us.2 ]
  %651 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %646
  store float %650, float* %651, align 4, !tbaa !372
  %indvars.iv.next103.2 = add nuw nsw i64 %indvars.iv102.2, 1
  %652 = add nuw nsw i32 %645, 1
  %exitcond104.2 = icmp eq i64 %indvars.iv.next103.2, 226
  br i1 %exitcond104.2, label %for_end6.us-lcssa.us.us.2, label %for_body5.us.us.2, !prof !34

for_end6.us-lcssa.us.us.2:                        ; preds = %if_end.us.us.2
  %653 = add nuw nsw i64 %8, 678
  %654 = add nsw i64 %17, 150528
  br label %for_body5.us.us.3

for_body5.us.us.3:                                ; preds = %if_end.us.us.3, %for_end6.us-lcssa.us.us.2
  %indvars.iv102.3 = phi i64 [ 0, %for_end6.us-lcssa.us.us.2 ], [ %indvars.iv.next103.3, %if_end.us.us.3 ]
  %655 = phi i32 [ 0, %for_end6.us-lcssa.us.us.2 ], [ %662, %if_end.us.us.3 ]
  %656 = add nuw nsw i64 %653, %indvars.iv102.3
  %trunc.us.us.3 = trunc i32 %655 to i31
  switch i31 %trunc.us.us.3, label %if_then.us.us.3 [
    i31 225, label %if_end.us.us.3
    i31 0, label %if_end.us.us.3
  ]

if_then.us.us.3:                                  ; preds = %for_body5.us.us.3
  %657 = add nsw i64 %654, %indvars.iv102.3
  %658 = getelementptr inbounds float, float* %7, i64 %657
  %659 = load float, float* %658, align 4, !tbaa !369
  br label %if_end.us.us.3

if_end.us.us.3:                                   ; preds = %if_then.us.us.3, %for_body5.us.us.3, %for_body5.us.us.3
  %660 = phi float [ %659, %if_then.us.us.3 ], [ 0.000000e+00, %for_body5.us.us.3 ], [ 0.000000e+00, %for_body5.us.us.3 ]
  %661 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %656
  store float %660, float* %661, align 4, !tbaa !372
  %indvars.iv.next103.3 = add nuw nsw i64 %indvars.iv102.3, 1
  %662 = add nuw nsw i32 %655, 1
  %exitcond104.3 = icmp eq i64 %indvars.iv.next103.3, 226
  br i1 %exitcond104.3, label %for_end6.us-lcssa.us.us.3, label %for_body5.us.us.3, !prof !34

for_end6.us-lcssa.us.us.3:                        ; preds = %if_end.us.us.3
  %663 = add nuw nsw i64 %8, 904
  %664 = add nsw i64 %17, 200704
  br label %for_body5.us.us.4

for_body5.us.us.4:                                ; preds = %if_end.us.us.4, %for_end6.us-lcssa.us.us.3
  %indvars.iv102.4 = phi i64 [ 0, %for_end6.us-lcssa.us.us.3 ], [ %indvars.iv.next103.4, %if_end.us.us.4 ]
  %665 = phi i32 [ 0, %for_end6.us-lcssa.us.us.3 ], [ %672, %if_end.us.us.4 ]
  %666 = add nuw nsw i64 %663, %indvars.iv102.4
  %trunc.us.us.4 = trunc i32 %665 to i31
  switch i31 %trunc.us.us.4, label %if_then.us.us.4 [
    i31 225, label %if_end.us.us.4
    i31 0, label %if_end.us.us.4
  ]

if_then.us.us.4:                                  ; preds = %for_body5.us.us.4
  %667 = add nsw i64 %664, %indvars.iv102.4
  %668 = getelementptr inbounds float, float* %7, i64 %667
  %669 = load float, float* %668, align 4, !tbaa !369
  br label %if_end.us.us.4

if_end.us.us.4:                                   ; preds = %if_then.us.us.4, %for_body5.us.us.4, %for_body5.us.us.4
  %670 = phi float [ %669, %if_then.us.us.4 ], [ 0.000000e+00, %for_body5.us.us.4 ], [ 0.000000e+00, %for_body5.us.us.4 ]
  %671 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %666
  store float %670, float* %671, align 4, !tbaa !372
  %indvars.iv.next103.4 = add nuw nsw i64 %indvars.iv102.4, 1
  %672 = add nuw nsw i32 %665, 1
  %exitcond104.4 = icmp eq i64 %indvars.iv.next103.4, 226
  br i1 %exitcond104.4, label %for_end6.us-lcssa.us.us.4, label %for_body5.us.us.4, !prof !34

for_end6.us-lcssa.us.us.4:                        ; preds = %if_end.us.us.4
  %673 = add nuw nsw i64 %8, 1130
  %674 = add nsw i64 %17, 250880
  br label %for_body5.us.us.5

for_body5.us.us.5:                                ; preds = %if_end.us.us.5, %for_end6.us-lcssa.us.us.4
  %indvars.iv102.5 = phi i64 [ 0, %for_end6.us-lcssa.us.us.4 ], [ %indvars.iv.next103.5, %if_end.us.us.5 ]
  %675 = phi i32 [ 0, %for_end6.us-lcssa.us.us.4 ], [ %682, %if_end.us.us.5 ]
  %676 = add nuw nsw i64 %673, %indvars.iv102.5
  %trunc.us.us.5 = trunc i32 %675 to i31
  switch i31 %trunc.us.us.5, label %if_then.us.us.5 [
    i31 225, label %if_end.us.us.5
    i31 0, label %if_end.us.us.5
  ]

if_then.us.us.5:                                  ; preds = %for_body5.us.us.5
  %677 = add nsw i64 %674, %indvars.iv102.5
  %678 = getelementptr inbounds float, float* %7, i64 %677
  %679 = load float, float* %678, align 4, !tbaa !369
  br label %if_end.us.us.5

if_end.us.us.5:                                   ; preds = %if_then.us.us.5, %for_body5.us.us.5, %for_body5.us.us.5
  %680 = phi float [ %679, %if_then.us.us.5 ], [ 0.000000e+00, %for_body5.us.us.5 ], [ 0.000000e+00, %for_body5.us.us.5 ]
  %681 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %676
  store float %680, float* %681, align 4, !tbaa !372
  %indvars.iv.next103.5 = add nuw nsw i64 %indvars.iv102.5, 1
  %682 = add nuw nsw i32 %675, 1
  %exitcond104.5 = icmp eq i64 %indvars.iv.next103.5, 226
  br i1 %exitcond104.5, label %for_end6.us-lcssa.us.us.5, label %for_body5.us.us.5, !prof !34

for_end6.us-lcssa.us.us.5:                        ; preds = %if_end.us.us.5
  %683 = add nuw nsw i64 %8, 1356
  %684 = add nsw i64 %17, 301056
  br label %for_body5.us.us.6

for_body5.us.us.6:                                ; preds = %if_end.us.us.6, %for_end6.us-lcssa.us.us.5
  %indvars.iv102.6 = phi i64 [ 0, %for_end6.us-lcssa.us.us.5 ], [ %indvars.iv.next103.6, %if_end.us.us.6 ]
  %685 = phi i32 [ 0, %for_end6.us-lcssa.us.us.5 ], [ %692, %if_end.us.us.6 ]
  %686 = add nuw nsw i64 %683, %indvars.iv102.6
  %trunc.us.us.6 = trunc i32 %685 to i31
  switch i31 %trunc.us.us.6, label %if_then.us.us.6 [
    i31 225, label %if_end.us.us.6
    i31 0, label %if_end.us.us.6
  ]

if_then.us.us.6:                                  ; preds = %for_body5.us.us.6
  %687 = add nsw i64 %684, %indvars.iv102.6
  %688 = getelementptr inbounds float, float* %7, i64 %687
  %689 = load float, float* %688, align 4, !tbaa !369
  br label %if_end.us.us.6

if_end.us.us.6:                                   ; preds = %if_then.us.us.6, %for_body5.us.us.6, %for_body5.us.us.6
  %690 = phi float [ %689, %if_then.us.us.6 ], [ 0.000000e+00, %for_body5.us.us.6 ], [ 0.000000e+00, %for_body5.us.us.6 ]
  %691 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %686
  store float %690, float* %691, align 4, !tbaa !372
  %indvars.iv.next103.6 = add nuw nsw i64 %indvars.iv102.6, 1
  %692 = add nuw nsw i32 %685, 1
  %exitcond104.6 = icmp eq i64 %indvars.iv.next103.6, 226
  br i1 %exitcond104.6, label %for_end6.us-lcssa.us.us.6, label %for_body5.us.us.6, !prof !34

for_end6.us-lcssa.us.us.6:                        ; preds = %if_end.us.us.6
  %693 = add nuw nsw i64 %8, 1582
  %694 = add nsw i64 %17, 351232
  br label %for_body5.us.us.7

for_body5.us.us.7:                                ; preds = %if_end.us.us.7, %for_end6.us-lcssa.us.us.6
  %indvars.iv102.7 = phi i64 [ 0, %for_end6.us-lcssa.us.us.6 ], [ %indvars.iv.next103.7, %if_end.us.us.7 ]
  %695 = phi i32 [ 0, %for_end6.us-lcssa.us.us.6 ], [ %702, %if_end.us.us.7 ]
  %696 = add nuw nsw i64 %693, %indvars.iv102.7
  %trunc.us.us.7 = trunc i32 %695 to i31
  switch i31 %trunc.us.us.7, label %if_then.us.us.7 [
    i31 225, label %if_end.us.us.7
    i31 0, label %if_end.us.us.7
  ]

if_then.us.us.7:                                  ; preds = %for_body5.us.us.7
  %697 = add nsw i64 %694, %indvars.iv102.7
  %698 = getelementptr inbounds float, float* %7, i64 %697
  %699 = load float, float* %698, align 4, !tbaa !369
  br label %if_end.us.us.7

if_end.us.us.7:                                   ; preds = %if_then.us.us.7, %for_body5.us.us.7, %for_body5.us.us.7
  %700 = phi float [ %699, %if_then.us.us.7 ], [ 0.000000e+00, %for_body5.us.us.7 ], [ 0.000000e+00, %for_body5.us.us.7 ]
  %701 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %696
  store float %700, float* %701, align 4, !tbaa !372
  %indvars.iv.next103.7 = add nuw nsw i64 %indvars.iv102.7, 1
  %702 = add nuw nsw i32 %695, 1
  %exitcond104.7 = icmp eq i64 %indvars.iv.next103.7, 226
  br i1 %exitcond104.7, label %for_end6.us-lcssa.us.us.7, label %for_body5.us.us.7, !prof !34

for_end6.us-lcssa.us.us.7:                        ; preds = %if_end.us.us.7
  %703 = add nuw nsw i64 %8, 1808
  %704 = add nsw i64 %17, 401408
  br label %for_body5.us.us.8

for_body5.us.us.8:                                ; preds = %if_end.us.us.8, %for_end6.us-lcssa.us.us.7
  %indvars.iv102.8 = phi i64 [ 0, %for_end6.us-lcssa.us.us.7 ], [ %indvars.iv.next103.8, %if_end.us.us.8 ]
  %705 = phi i32 [ 0, %for_end6.us-lcssa.us.us.7 ], [ %712, %if_end.us.us.8 ]
  %706 = add nuw nsw i64 %703, %indvars.iv102.8
  %trunc.us.us.8 = trunc i32 %705 to i31
  switch i31 %trunc.us.us.8, label %if_then.us.us.8 [
    i31 225, label %if_end.us.us.8
    i31 0, label %if_end.us.us.8
  ]

if_then.us.us.8:                                  ; preds = %for_body5.us.us.8
  %707 = add nsw i64 %704, %indvars.iv102.8
  %708 = getelementptr inbounds float, float* %7, i64 %707
  %709 = load float, float* %708, align 4, !tbaa !369
  br label %if_end.us.us.8

if_end.us.us.8:                                   ; preds = %if_then.us.us.8, %for_body5.us.us.8, %for_body5.us.us.8
  %710 = phi float [ %709, %if_then.us.us.8 ], [ 0.000000e+00, %for_body5.us.us.8 ], [ 0.000000e+00, %for_body5.us.us.8 ]
  %711 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %706
  store float %710, float* %711, align 4, !tbaa !372
  %indvars.iv.next103.8 = add nuw nsw i64 %indvars.iv102.8, 1
  %712 = add nuw nsw i32 %705, 1
  %exitcond104.8 = icmp eq i64 %indvars.iv.next103.8, 226
  br i1 %exitcond104.8, label %for_end6.us-lcssa.us.us.8, label %for_body5.us.us.8, !prof !34

for_end6.us-lcssa.us.us.8:                        ; preds = %if_end.us.us.8
  %713 = add nuw nsw i64 %8, 2034
  %714 = add nsw i64 %17, 451584
  br label %for_body5.us.us.9

for_body5.us.us.9:                                ; preds = %if_end.us.us.9, %for_end6.us-lcssa.us.us.8
  %indvars.iv102.9 = phi i64 [ 0, %for_end6.us-lcssa.us.us.8 ], [ %indvars.iv.next103.9, %if_end.us.us.9 ]
  %715 = phi i32 [ 0, %for_end6.us-lcssa.us.us.8 ], [ %722, %if_end.us.us.9 ]
  %716 = add nuw nsw i64 %713, %indvars.iv102.9
  %trunc.us.us.9 = trunc i32 %715 to i31
  switch i31 %trunc.us.us.9, label %if_then.us.us.9 [
    i31 225, label %if_end.us.us.9
    i31 0, label %if_end.us.us.9
  ]

if_then.us.us.9:                                  ; preds = %for_body5.us.us.9
  %717 = add nsw i64 %714, %indvars.iv102.9
  %718 = getelementptr inbounds float, float* %7, i64 %717
  %719 = load float, float* %718, align 4, !tbaa !369
  br label %if_end.us.us.9

if_end.us.us.9:                                   ; preds = %if_then.us.us.9, %for_body5.us.us.9, %for_body5.us.us.9
  %720 = phi float [ %719, %if_then.us.us.9 ], [ 0.000000e+00, %for_body5.us.us.9 ], [ 0.000000e+00, %for_body5.us.us.9 ]
  %721 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %716
  store float %720, float* %721, align 4, !tbaa !372
  %indvars.iv.next103.9 = add nuw nsw i64 %indvars.iv102.9, 1
  %722 = add nuw nsw i32 %715, 1
  %exitcond104.9 = icmp eq i64 %indvars.iv.next103.9, 226
  br i1 %exitcond104.9, label %for_end6.us-lcssa.us.us.9, label %for_body5.us.us.9, !prof !34

for_end6.us-lcssa.us.us.9:                        ; preds = %if_end.us.us.9
  %723 = add nuw nsw i64 %8, 2260
  %724 = add nsw i64 %17, 501760
  br label %for_body5.us.us.10

for_body5.us.us.10:                               ; preds = %if_end.us.us.10, %for_end6.us-lcssa.us.us.9
  %indvars.iv102.10 = phi i64 [ 0, %for_end6.us-lcssa.us.us.9 ], [ %indvars.iv.next103.10, %if_end.us.us.10 ]
  %725 = phi i32 [ 0, %for_end6.us-lcssa.us.us.9 ], [ %732, %if_end.us.us.10 ]
  %726 = add nuw nsw i64 %723, %indvars.iv102.10
  %trunc.us.us.10 = trunc i32 %725 to i31
  switch i31 %trunc.us.us.10, label %if_then.us.us.10 [
    i31 225, label %if_end.us.us.10
    i31 0, label %if_end.us.us.10
  ]

if_then.us.us.10:                                 ; preds = %for_body5.us.us.10
  %727 = add nsw i64 %724, %indvars.iv102.10
  %728 = getelementptr inbounds float, float* %7, i64 %727
  %729 = load float, float* %728, align 4, !tbaa !369
  br label %if_end.us.us.10

if_end.us.us.10:                                  ; preds = %if_then.us.us.10, %for_body5.us.us.10, %for_body5.us.us.10
  %730 = phi float [ %729, %if_then.us.us.10 ], [ 0.000000e+00, %for_body5.us.us.10 ], [ 0.000000e+00, %for_body5.us.us.10 ]
  %731 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %726
  store float %730, float* %731, align 4, !tbaa !372
  %indvars.iv.next103.10 = add nuw nsw i64 %indvars.iv102.10, 1
  %732 = add nuw nsw i32 %725, 1
  %exitcond104.10 = icmp eq i64 %indvars.iv.next103.10, 226
  br i1 %exitcond104.10, label %for_end6.us-lcssa.us.us.10, label %for_body5.us.us.10, !prof !34

for_end6.us-lcssa.us.us.10:                       ; preds = %if_end.us.us.10
  %733 = add nuw nsw i64 %8, 2486
  %734 = add nsw i64 %17, 551936
  br label %for_body5.us.us.11

for_body5.us.us.11:                               ; preds = %if_end.us.us.11, %for_end6.us-lcssa.us.us.10
  %indvars.iv102.11 = phi i64 [ 0, %for_end6.us-lcssa.us.us.10 ], [ %indvars.iv.next103.11, %if_end.us.us.11 ]
  %735 = phi i32 [ 0, %for_end6.us-lcssa.us.us.10 ], [ %742, %if_end.us.us.11 ]
  %736 = add nuw nsw i64 %733, %indvars.iv102.11
  %trunc.us.us.11 = trunc i32 %735 to i31
  switch i31 %trunc.us.us.11, label %if_then.us.us.11 [
    i31 225, label %if_end.us.us.11
    i31 0, label %if_end.us.us.11
  ]

if_then.us.us.11:                                 ; preds = %for_body5.us.us.11
  %737 = add nsw i64 %734, %indvars.iv102.11
  %738 = getelementptr inbounds float, float* %7, i64 %737
  %739 = load float, float* %738, align 4, !tbaa !369
  br label %if_end.us.us.11

if_end.us.us.11:                                  ; preds = %if_then.us.us.11, %for_body5.us.us.11, %for_body5.us.us.11
  %740 = phi float [ %739, %if_then.us.us.11 ], [ 0.000000e+00, %for_body5.us.us.11 ], [ 0.000000e+00, %for_body5.us.us.11 ]
  %741 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %736
  store float %740, float* %741, align 4, !tbaa !372
  %indvars.iv.next103.11 = add nuw nsw i64 %indvars.iv102.11, 1
  %742 = add nuw nsw i32 %735, 1
  %exitcond104.11 = icmp eq i64 %indvars.iv.next103.11, 226
  br i1 %exitcond104.11, label %for_end6.us-lcssa.us.us.11, label %for_body5.us.us.11, !prof !34

for_end6.us-lcssa.us.us.11:                       ; preds = %if_end.us.us.11
  %743 = add nuw nsw i64 %8, 2712
  %744 = add nsw i64 %17, 602112
  br label %for_body5.us.us.12

for_body5.us.us.12:                               ; preds = %if_end.us.us.12, %for_end6.us-lcssa.us.us.11
  %indvars.iv102.12 = phi i64 [ 0, %for_end6.us-lcssa.us.us.11 ], [ %indvars.iv.next103.12, %if_end.us.us.12 ]
  %745 = phi i32 [ 0, %for_end6.us-lcssa.us.us.11 ], [ %752, %if_end.us.us.12 ]
  %746 = add nuw nsw i64 %743, %indvars.iv102.12
  %trunc.us.us.12 = trunc i32 %745 to i31
  switch i31 %trunc.us.us.12, label %if_then.us.us.12 [
    i31 225, label %if_end.us.us.12
    i31 0, label %if_end.us.us.12
  ]

if_then.us.us.12:                                 ; preds = %for_body5.us.us.12
  %747 = add nsw i64 %744, %indvars.iv102.12
  %748 = getelementptr inbounds float, float* %7, i64 %747
  %749 = load float, float* %748, align 4, !tbaa !369
  br label %if_end.us.us.12

if_end.us.us.12:                                  ; preds = %if_then.us.us.12, %for_body5.us.us.12, %for_body5.us.us.12
  %750 = phi float [ %749, %if_then.us.us.12 ], [ 0.000000e+00, %for_body5.us.us.12 ], [ 0.000000e+00, %for_body5.us.us.12 ]
  %751 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %746
  store float %750, float* %751, align 4, !tbaa !372
  %indvars.iv.next103.12 = add nuw nsw i64 %indvars.iv102.12, 1
  %752 = add nuw nsw i32 %745, 1
  %exitcond104.12 = icmp eq i64 %indvars.iv.next103.12, 226
  br i1 %exitcond104.12, label %for_end6.us-lcssa.us.us.12, label %for_body5.us.us.12, !prof !34

for_end6.us-lcssa.us.us.12:                       ; preds = %if_end.us.us.12
  %753 = add nuw nsw i64 %8, 2938
  %754 = add nsw i64 %17, 652288
  br label %for_body5.us.us.13

for_body5.us.us.13:                               ; preds = %if_end.us.us.13, %for_end6.us-lcssa.us.us.12
  %indvars.iv102.13 = phi i64 [ 0, %for_end6.us-lcssa.us.us.12 ], [ %indvars.iv.next103.13, %if_end.us.us.13 ]
  %755 = phi i32 [ 0, %for_end6.us-lcssa.us.us.12 ], [ %762, %if_end.us.us.13 ]
  %756 = add nuw nsw i64 %753, %indvars.iv102.13
  %trunc.us.us.13 = trunc i32 %755 to i31
  switch i31 %trunc.us.us.13, label %if_then.us.us.13 [
    i31 225, label %if_end.us.us.13
    i31 0, label %if_end.us.us.13
  ]

if_then.us.us.13:                                 ; preds = %for_body5.us.us.13
  %757 = add nsw i64 %754, %indvars.iv102.13
  %758 = getelementptr inbounds float, float* %7, i64 %757
  %759 = load float, float* %758, align 4, !tbaa !369
  br label %if_end.us.us.13

if_end.us.us.13:                                  ; preds = %if_then.us.us.13, %for_body5.us.us.13, %for_body5.us.us.13
  %760 = phi float [ %759, %if_then.us.us.13 ], [ 0.000000e+00, %for_body5.us.us.13 ], [ 0.000000e+00, %for_body5.us.us.13 ]
  %761 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %756
  store float %760, float* %761, align 4, !tbaa !372
  %indvars.iv.next103.13 = add nuw nsw i64 %indvars.iv102.13, 1
  %762 = add nuw nsw i32 %755, 1
  %exitcond104.13 = icmp eq i64 %indvars.iv.next103.13, 226
  br i1 %exitcond104.13, label %for_end6.us-lcssa.us.us.13, label %for_body5.us.us.13, !prof !34

for_end6.us-lcssa.us.us.13:                       ; preds = %if_end.us.us.13
  %763 = add nuw nsw i64 %8, 3164
  %764 = add nsw i64 %17, 702464
  br label %for_body5.us.us.14

for_body5.us.us.14:                               ; preds = %if_end.us.us.14, %for_end6.us-lcssa.us.us.13
  %indvars.iv102.14 = phi i64 [ 0, %for_end6.us-lcssa.us.us.13 ], [ %indvars.iv.next103.14, %if_end.us.us.14 ]
  %765 = phi i32 [ 0, %for_end6.us-lcssa.us.us.13 ], [ %772, %if_end.us.us.14 ]
  %766 = add nuw nsw i64 %763, %indvars.iv102.14
  %trunc.us.us.14 = trunc i32 %765 to i31
  switch i31 %trunc.us.us.14, label %if_then.us.us.14 [
    i31 225, label %if_end.us.us.14
    i31 0, label %if_end.us.us.14
  ]

if_then.us.us.14:                                 ; preds = %for_body5.us.us.14
  %767 = add nsw i64 %764, %indvars.iv102.14
  %768 = getelementptr inbounds float, float* %7, i64 %767
  %769 = load float, float* %768, align 4, !tbaa !369
  br label %if_end.us.us.14

if_end.us.us.14:                                  ; preds = %if_then.us.us.14, %for_body5.us.us.14, %for_body5.us.us.14
  %770 = phi float [ %769, %if_then.us.us.14 ], [ 0.000000e+00, %for_body5.us.us.14 ], [ 0.000000e+00, %for_body5.us.us.14 ]
  %771 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %766
  store float %770, float* %771, align 4, !tbaa !372
  %indvars.iv.next103.14 = add nuw nsw i64 %indvars.iv102.14, 1
  %772 = add nuw nsw i32 %765, 1
  %exitcond104.14 = icmp eq i64 %indvars.iv.next103.14, 226
  br i1 %exitcond104.14, label %for_end6.us-lcssa.us.us.14, label %for_body5.us.us.14, !prof !34

for_end6.us-lcssa.us.us.14:                       ; preds = %if_end.us.us.14
  %773 = add nuw nsw i64 %8, 3390
  %774 = add nsw i64 %17, 752640
  br label %for_body5.us.us.15

for_body5.us.us.15:                               ; preds = %if_end.us.us.15, %for_end6.us-lcssa.us.us.14
  %indvars.iv102.15 = phi i64 [ 0, %for_end6.us-lcssa.us.us.14 ], [ %indvars.iv.next103.15, %if_end.us.us.15 ]
  %775 = phi i32 [ 0, %for_end6.us-lcssa.us.us.14 ], [ %782, %if_end.us.us.15 ]
  %776 = add nuw nsw i64 %773, %indvars.iv102.15
  %trunc.us.us.15 = trunc i32 %775 to i31
  switch i31 %trunc.us.us.15, label %if_then.us.us.15 [
    i31 225, label %if_end.us.us.15
    i31 0, label %if_end.us.us.15
  ]

if_then.us.us.15:                                 ; preds = %for_body5.us.us.15
  %777 = add nsw i64 %774, %indvars.iv102.15
  %778 = getelementptr inbounds float, float* %7, i64 %777
  %779 = load float, float* %778, align 4, !tbaa !369
  br label %if_end.us.us.15

if_end.us.us.15:                                  ; preds = %if_then.us.us.15, %for_body5.us.us.15, %for_body5.us.us.15
  %780 = phi float [ %779, %if_then.us.us.15 ], [ 0.000000e+00, %for_body5.us.us.15 ], [ 0.000000e+00, %for_body5.us.us.15 ]
  %781 = getelementptr inbounds [3268864 x float], [3268864 x float]* %6, i64 0, i64 %776
  store float %780, float* %781, align 4, !tbaa !372
  %indvars.iv.next103.15 = add nuw nsw i64 %indvars.iv102.15, 1
  %782 = add nuw nsw i32 %775, 1
  %exitcond104.15 = icmp eq i64 %indvars.iv.next103.15, 226
  br i1 %exitcond104.15, label %for_end3, label %for_body5.us.us.15, !prof !34
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
  %wide.load = load <4 x float>, <4 x float>* %5, align 4, !tbaa !396
  %6 = getelementptr inbounds float, float* %4, i64 4
  %7 = bitcast float* %6 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %7, align 4, !tbaa !396
  %8 = fcmp ogt <4 x float> %wide.load, zeroinitializer
  %9 = fcmp ogt <4 x float> %wide.load2, zeroinitializer
  %10 = select <4 x i1> %8, <4 x float> %wide.load, <4 x float> zeroinitializer
  %11 = select <4 x i1> %9, <4 x float> %wide.load2, <4 x float> zeroinitializer
  %12 = getelementptr inbounds float, float* %3, i64 %index
  %13 = bitcast float* %12 to <4 x float>*
  store <4 x float> %10, <4 x float>* %13, align 4, !tbaa !399
  %14 = getelementptr inbounds float, float* %12, i64 4
  %15 = bitcast float* %14 to <4 x float>*
  store <4 x float> %11, <4 x float>* %15, align 4, !tbaa !399
  %index.next = add i64 %index, 8
  %16 = icmp eq i64 %index.next, 4096
  br i1 %16, label %for_end, label %vector.body, !llvm.loop !402

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
define private fastcc void @fused_nn_conv2d_4_compute_(i8* noalias nocapture readonly, i8* noalias readonly, i8* noalias) unnamed_addr #3 {
entry:
  %3 = alloca [8 x <32 x float>], align 16
  %4 = alloca [56 x <32 x float>], align 16
  %5 = alloca [9216 x <32 x float>], align 16
  %6 = alloca [430592 x float], align 16
  %7 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar93 = phi i64 [ 0, %entry ], [ %indvar.next94, %for_end3 ]
  %8 = mul nuw nsw i64 %indvar93, 1856
  %9 = trunc i64 %indvar93 to i32
  %10 = urem i32 %9, 58
  %11 = udiv i32 %9, 58
  %.off = add nsw i32 %10, -1
  %12 = icmp ult i32 %.off, 56
  br i1 %12, label %for_begin4.preheader.us.preheader, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep100 = getelementptr [430592 x float], [430592 x float]* %6, i64 0, i64 %8
  %scevgep100101 = bitcast float* %scevgep100 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep100101, i8 0, i64 7424, i1 false)
  br label %for_end3

for_begin4.preheader.us.preheader:                ; preds = %for_begin1.preheader
  %13 = mul nsw i32 %11, 100352
  %14 = add nsw i32 %13, -57
  %15 = mul nuw nsw i32 %10, 56
  %16 = add i32 %14, %15
  %17 = sext i32 %16 to i64
  br label %for_begin4.preheader.us

for_begin4.preheader.us:                          ; preds = %for_end6.us-lcssa.us.us, %for_begin4.preheader.us.preheader
  %indvars.iv105 = phi i64 [ 0, %for_begin4.preheader.us.preheader ], [ %indvars.iv.next106, %for_end6.us-lcssa.us.us ]
  %18 = mul nuw nsw i64 %indvars.iv105, 58
  %19 = add nuw nsw i64 %18, %8
  %20 = mul nuw nsw i64 %indvars.iv105, 3136
  %21 = add nsw i64 %20, %17
  br label %for_body5.us.us

for_body5.us.us:                                  ; preds = %if_end.us.us, %for_begin4.preheader.us
  %indvars.iv102 = phi i64 [ 0, %for_begin4.preheader.us ], [ %indvars.iv.next103, %if_end.us.us ]
  %22 = phi i32 [ 0, %for_begin4.preheader.us ], [ %29, %if_end.us.us ]
  %23 = add nuw nsw i64 %19, %indvars.iv102
  %trunc.us.us = trunc i32 %22 to i31
  switch i31 %trunc.us.us, label %if_then.us.us [
    i31 57, label %if_end.us.us
    i31 0, label %if_end.us.us
  ]

if_then.us.us:                                    ; preds = %for_body5.us.us
  %24 = add nsw i64 %21, %indvars.iv102
  %25 = getelementptr inbounds float, float* %7, i64 %24
  %26 = load float, float* %25, align 4, !tbaa !403
  br label %if_end.us.us

if_end.us.us:                                     ; preds = %if_then.us.us, %for_body5.us.us, %for_body5.us.us
  %27 = phi float [ %26, %if_then.us.us ], [ 0.000000e+00, %for_body5.us.us ], [ 0.000000e+00, %for_body5.us.us ]
  %28 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %23
  store float %27, float* %28, align 4, !tbaa !406
  %indvars.iv.next103 = add nuw nsw i64 %indvars.iv102, 1
  %29 = add nuw nsw i32 %22, 1
  %exitcond104 = icmp eq i64 %indvars.iv.next103, 58
  br i1 %exitcond104, label %for_end6.us-lcssa.us.us, label %for_body5.us.us, !prof !34

for_end6.us-lcssa.us.us:                          ; preds = %if_end.us.us
  %indvars.iv.next106 = add nuw nsw i64 %indvars.iv105, 1
  %exitcond107 = icmp eq i64 %indvars.iv.next106, 32
  br i1 %exitcond107, label %for_end3, label %for_begin4.preheader.us, !prof !34

for_begin7.preheader:                             ; preds = %for_end3
  %30 = bitcast [8 x <32 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0
  %31 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_end3:                                         ; preds = %for_end6.us-lcssa.us.us, %for_begin4.preheader.preheader
  %indvar.next94 = add nuw nsw i64 %indvar93, 1
  %exitcond108 = icmp eq i64 %indvar.next94, 232
  br i1 %exitcond108, label %for_begin7.preheader, label %for_begin1.preheader, !prof !34

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %32 = phi i32 [ 0, %for_begin7.preheader ], [ %62, %for_end12 ]
  %33 = urem i32 %32, 3
  %34 = mul nuw nsw i32 %33, 3072
  %35 = udiv i32 %32, 3
  %36 = mul nsw i32 %35, 36864
  %37 = add nuw i32 %34, %36
  %38 = mul nuw nsw i32 %33, 3
  %39 = or i32 %38, %36
  %40 = zext i32 %39 to i64
  %41 = sext i32 %37 to i64
  br label %for_begin13.preheader

for_begin19.preheader:                            ; preds = %for_end12
  %42 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 32
  %43 = bitcast float* %42 to <32 x float>*
  %44 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 64
  %45 = bitcast float* %44 to <32 x float>*
  %46 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 96
  %47 = bitcast float* %46 to <32 x float>*
  %48 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 128
  %49 = bitcast float* %48 to <32 x float>*
  %50 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 160
  %51 = bitcast float* %50 to <32 x float>*
  %52 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 192
  %53 = bitcast float* %52 to <32 x float>*
  %54 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 224
  %55 = bitcast float* %54 to <32 x float>*
  %56 = bitcast i8* %2 to float*
  %57 = bitcast [8 x <32 x float>]* %3 to i8*
  br label %for_begin22.preheader

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvars.iv86 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next87, %for_end15 ]
  %58 = mul nuw nsw i64 %indvars.iv86, 9216
  %59 = add nuw nsw i64 %58, %41
  %60 = mul nuw nsw i64 %indvars.iv86, 288
  %61 = add nuw nsw i64 %60, %40
  br label %for_begin16.preheader

for_end12:                                        ; preds = %for_end15
  %62 = add nuw nsw i32 %32, 1
  %exitcond89 = icmp eq i32 %62, 24
  br i1 %exitcond89, label %for_begin19.preheader, label %for_begin10.preheader, !prof !34

for_begin16.preheader:                            ; preds = %for_end18, %for_begin13.preheader
  %indvars.iv83 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next84, %for_end18 ]
  %63 = shl i64 %indvars.iv83, 10
  %64 = add nuw nsw i64 %59, %63
  %65 = add nuw nsw i64 %61, %indvars.iv83
  br label %for_body17

for_end15:                                        ; preds = %for_end18
  %indvars.iv.next87 = add nuw nsw i64 %indvars.iv86, 1
  %exitcond88 = icmp eq i64 %indvars.iv.next87, 4
  br i1 %exitcond88, label %for_end12, label %for_begin13.preheader, !prof !34

for_body17:                                       ; preds = %for_body17, %for_begin16.preheader
  %indvars.iv80 = phi i64 [ 0, %for_begin16.preheader ], [ %indvars.iv.next81, %for_body17 ]
  %66 = shl i64 %indvars.iv80, 5
  %67 = add nuw nsw i64 %64, %66
  %68 = mul nuw nsw i64 %indvars.iv80, 9
  %69 = add nuw nsw i64 %65, %68
  %70 = add nuw nsw i64 %69, 1152
  %71 = add nuw nsw i64 %69, 2304
  %72 = add nuw nsw i64 %69, 3456
  %73 = add nuw nsw i64 %69, 4608
  %74 = add nuw nsw i64 %69, 5760
  %75 = add nuw nsw i64 %69, 6912
  %76 = add nuw nsw i64 %69, 8064
  %77 = add nuw nsw i64 %69, 9216
  %78 = add nuw nsw i64 %69, 10368
  %79 = add nuw nsw i64 %69, 11520
  %80 = add nuw nsw i64 %69, 12672
  %81 = add nuw nsw i64 %69, 13824
  %82 = add nuw nsw i64 %69, 14976
  %83 = add nuw nsw i64 %69, 16128
  %84 = add nuw nsw i64 %69, 17280
  %85 = add nuw nsw i64 %69, 18432
  %86 = add nuw nsw i64 %69, 19584
  %87 = add nuw nsw i64 %69, 20736
  %88 = add nuw nsw i64 %69, 21888
  %89 = add nuw nsw i64 %69, 23040
  %90 = add nuw nsw i64 %69, 24192
  %91 = add nuw nsw i64 %69, 25344
  %92 = add nuw nsw i64 %69, 26496
  %93 = add nuw nsw i64 %69, 27648
  %94 = add nuw nsw i64 %69, 28800
  %95 = add nuw nsw i64 %69, 29952
  %96 = add nuw nsw i64 %69, 31104
  %97 = add nuw nsw i64 %69, 32256
  %98 = add nuw nsw i64 %69, 33408
  %99 = add nuw nsw i64 %69, 34560
  %100 = add nuw nsw i64 %69, 35712
  %101 = getelementptr inbounds float, float* %31, i64 %69
  %102 = load float, float* %101, align 4, !tbaa !409
  %103 = insertelement <32 x float> undef, float %102, i32 0
  %104 = getelementptr inbounds float, float* %31, i64 %70
  %105 = load float, float* %104, align 4, !tbaa !409
  %106 = insertelement <32 x float> %103, float %105, i32 1
  %107 = getelementptr inbounds float, float* %31, i64 %71
  %108 = load float, float* %107, align 4, !tbaa !409
  %109 = insertelement <32 x float> %106, float %108, i32 2
  %110 = getelementptr inbounds float, float* %31, i64 %72
  %111 = load float, float* %110, align 4, !tbaa !409
  %112 = insertelement <32 x float> %109, float %111, i32 3
  %113 = getelementptr inbounds float, float* %31, i64 %73
  %114 = load float, float* %113, align 4, !tbaa !409
  %115 = insertelement <32 x float> %112, float %114, i32 4
  %116 = getelementptr inbounds float, float* %31, i64 %74
  %117 = load float, float* %116, align 4, !tbaa !409
  %118 = insertelement <32 x float> %115, float %117, i32 5
  %119 = getelementptr inbounds float, float* %31, i64 %75
  %120 = load float, float* %119, align 4, !tbaa !409
  %121 = insertelement <32 x float> %118, float %120, i32 6
  %122 = getelementptr inbounds float, float* %31, i64 %76
  %123 = load float, float* %122, align 4, !tbaa !409
  %124 = insertelement <32 x float> %121, float %123, i32 7
  %125 = getelementptr inbounds float, float* %31, i64 %77
  %126 = load float, float* %125, align 4, !tbaa !409
  %127 = insertelement <32 x float> %124, float %126, i32 8
  %128 = getelementptr inbounds float, float* %31, i64 %78
  %129 = load float, float* %128, align 4, !tbaa !409
  %130 = insertelement <32 x float> %127, float %129, i32 9
  %131 = getelementptr inbounds float, float* %31, i64 %79
  %132 = load float, float* %131, align 4, !tbaa !409
  %133 = insertelement <32 x float> %130, float %132, i32 10
  %134 = getelementptr inbounds float, float* %31, i64 %80
  %135 = load float, float* %134, align 4, !tbaa !409
  %136 = insertelement <32 x float> %133, float %135, i32 11
  %137 = getelementptr inbounds float, float* %31, i64 %81
  %138 = load float, float* %137, align 4, !tbaa !409
  %139 = insertelement <32 x float> %136, float %138, i32 12
  %140 = getelementptr inbounds float, float* %31, i64 %82
  %141 = load float, float* %140, align 4, !tbaa !409
  %142 = insertelement <32 x float> %139, float %141, i32 13
  %143 = getelementptr inbounds float, float* %31, i64 %83
  %144 = load float, float* %143, align 4, !tbaa !409
  %145 = insertelement <32 x float> %142, float %144, i32 14
  %146 = getelementptr inbounds float, float* %31, i64 %84
  %147 = load float, float* %146, align 4, !tbaa !409
  %148 = insertelement <32 x float> %145, float %147, i32 15
  %149 = getelementptr inbounds float, float* %31, i64 %85
  %150 = load float, float* %149, align 4, !tbaa !409
  %151 = insertelement <32 x float> %148, float %150, i32 16
  %152 = getelementptr inbounds float, float* %31, i64 %86
  %153 = load float, float* %152, align 4, !tbaa !409
  %154 = insertelement <32 x float> %151, float %153, i32 17
  %155 = getelementptr inbounds float, float* %31, i64 %87
  %156 = load float, float* %155, align 4, !tbaa !409
  %157 = insertelement <32 x float> %154, float %156, i32 18
  %158 = getelementptr inbounds float, float* %31, i64 %88
  %159 = load float, float* %158, align 4, !tbaa !409
  %160 = insertelement <32 x float> %157, float %159, i32 19
  %161 = getelementptr inbounds float, float* %31, i64 %89
  %162 = load float, float* %161, align 4, !tbaa !409
  %163 = insertelement <32 x float> %160, float %162, i32 20
  %164 = getelementptr inbounds float, float* %31, i64 %90
  %165 = load float, float* %164, align 4, !tbaa !409
  %166 = insertelement <32 x float> %163, float %165, i32 21
  %167 = getelementptr inbounds float, float* %31, i64 %91
  %168 = load float, float* %167, align 4, !tbaa !409
  %169 = insertelement <32 x float> %166, float %168, i32 22
  %170 = getelementptr inbounds float, float* %31, i64 %92
  %171 = load float, float* %170, align 4, !tbaa !409
  %172 = insertelement <32 x float> %169, float %171, i32 23
  %173 = getelementptr inbounds float, float* %31, i64 %93
  %174 = load float, float* %173, align 4, !tbaa !409
  %175 = insertelement <32 x float> %172, float %174, i32 24
  %176 = getelementptr inbounds float, float* %31, i64 %94
  %177 = load float, float* %176, align 4, !tbaa !409
  %178 = insertelement <32 x float> %175, float %177, i32 25
  %179 = getelementptr inbounds float, float* %31, i64 %95
  %180 = load float, float* %179, align 4, !tbaa !409
  %181 = insertelement <32 x float> %178, float %180, i32 26
  %182 = getelementptr inbounds float, float* %31, i64 %96
  %183 = load float, float* %182, align 4, !tbaa !409
  %184 = insertelement <32 x float> %181, float %183, i32 27
  %185 = getelementptr inbounds float, float* %31, i64 %97
  %186 = load float, float* %185, align 4, !tbaa !409
  %187 = insertelement <32 x float> %184, float %186, i32 28
  %188 = getelementptr inbounds float, float* %31, i64 %98
  %189 = load float, float* %188, align 4, !tbaa !409
  %190 = insertelement <32 x float> %187, float %189, i32 29
  %191 = getelementptr inbounds float, float* %31, i64 %99
  %192 = load float, float* %191, align 4, !tbaa !409
  %193 = insertelement <32 x float> %190, float %192, i32 30
  %194 = getelementptr inbounds float, float* %31, i64 %100
  %195 = load float, float* %194, align 4, !tbaa !409
  %196 = insertelement <32 x float> %193, float %195, i32 31
  %197 = getelementptr inbounds [9216 x <32 x float>], [9216 x <32 x float>]* %5, i64 0, i64 0, i64 %67
  %198 = bitcast float* %197 to <32 x float>*
  store <32 x float> %196, <32 x float>* %198, align 16, !tbaa !412
  %indvars.iv.next81 = add nuw nsw i64 %indvars.iv80, 1
  %exitcond82 = icmp eq i64 %indvars.iv.next81, 32
  br i1 %exitcond82, label %for_end18, label %for_body17, !prof !34

for_end18:                                        ; preds = %for_body17
  %indvars.iv.next84 = add nuw nsw i64 %indvars.iv83, 1
  %exitcond85 = icmp eq i64 %indvars.iv.next84, 3
  br i1 %exitcond85, label %for_end15, label %for_begin16.preheader, !prof !34

for_begin22.preheader:                            ; preds = %for_end39, %for_begin19.preheader
  %199 = phi i32 [ 0, %for_begin19.preheader ], [ %328, %for_end39 ]
  %200 = urem i32 %199, 56
  %201 = udiv i32 %199, 56
  %202 = mul nsw i32 %201, 36864
  %203 = zext i32 %202 to i64
  %reass.mul = mul nuw nsw i32 %200, 1856
  %204 = zext i32 %reass.mul to i64
  %205 = mul nuw nsw i32 %200, 1856
  %reass.mul.1 = add nuw nsw i32 %205, 1856
  %206 = zext i32 %reass.mul.1 to i64
  %207 = mul nuw nsw i32 %200, 1856
  %reass.mul.2 = add nuw nsw i32 %207, 3712
  %208 = zext i32 %reass.mul.2 to i64
  br label %for_body23

for_end21:                                        ; preds = %for_end39
  ret void

for_begin37.preheader:                            ; preds = %for_begin34.preheader
  %209 = mul nuw nsw i32 %200, 56
  %210 = mul nsw i32 %201, 100352
  %211 = add nuw nsw i32 %210, %209
  %212 = zext i32 %211 to i64
  br label %for_begin40.preheader

for_body23:                                       ; preds = %for_begin34.preheader, %for_begin22.preheader
  %indvar = phi i64 [ 0, %for_begin22.preheader ], [ %indvar.next, %for_begin34.preheader ]
  %213 = shl i64 %indvar, 3
  %scevgep = getelementptr [56 x <32 x float>], [56 x <32 x float>]* %4, i64 0, i64 %213
  %scevgep71 = bitcast <32 x float>* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %57, i8 0, i64 1024, i1 false)
  br label %for_begin28.preheader

for_begin34.preheader:                            ; preds = %for_end33.2
  store <32 x float> %623, <32 x float>* %.sub, align 16, !tbaa !415
  store <32 x float> %624, <32 x float>* %43, align 16, !tbaa !415
  store <32 x float> %625, <32 x float>* %45, align 16, !tbaa !415
  store <32 x float> %626, <32 x float>* %47, align 16, !tbaa !415
  store <32 x float> %627, <32 x float>* %49, align 16, !tbaa !415
  store <32 x float> %628, <32 x float>* %51, align 16, !tbaa !415
  store <32 x float> %629, <32 x float>* %53, align 16, !tbaa !415
  store <32 x float> %635, <32 x float>* %55, align 16, !tbaa !415
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep71, i8* nonnull align 16 %30, i64 1024, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond72 = icmp eq i64 %indvar.next, 7
  br i1 %exitcond72, label %for_begin37.preheader, label %for_body23, !prof !34

for_begin28.preheader:                            ; preds = %for_end33.2, %for_body23
  %indvars.iv65 = phi i64 [ 0, %for_body23 ], [ %indvars.iv.next66, %for_end33.2 ]
  %.lcssa16.lcssa45 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %635, %for_end33.2 ]
  %.lcssa14.lcssa43 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %629, %for_end33.2 ]
  %.lcssa12.lcssa41 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %628, %for_end33.2 ]
  %.lcssa10.lcssa39 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %627, %for_end33.2 ]
  %.lcssa8.lcssa37 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %626, %for_end33.2 ]
  %.lcssa6.lcssa36 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %625, %for_end33.2 ]
  %.lcssa4.lcssa34 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %624, %for_end33.2 ]
  %.lcssa.lcssa32 = phi <32 x float> [ zeroinitializer, %for_body23 ], [ %623, %for_end33.2 ]
  %214 = mul nuw nsw i64 %indvars.iv65, 107648
  %215 = add nuw nsw i64 %214, %213
  %216 = mul nuw nsw i64 %indvars.iv65, 9216
  %217 = add nuw nsw i64 %216, %203
  %218 = add nsw i64 %215, %204
  %219 = trunc i64 %218 to i32
  br label %for_body32

for_body32:                                       ; preds = %for_body32, %for_begin28.preheader
  %indvars.iv = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next, %for_body32 ]
  %220 = phi <32 x float> [ %.lcssa16.lcssa45, %for_begin28.preheader ], [ %320, %for_body32 ]
  %221 = phi <32 x float> [ %.lcssa14.lcssa43, %for_begin28.preheader ], [ %314, %for_body32 ]
  %222 = phi <32 x float> [ %.lcssa12.lcssa41, %for_begin28.preheader ], [ %313, %for_body32 ]
  %223 = phi <32 x float> [ %.lcssa10.lcssa39, %for_begin28.preheader ], [ %312, %for_body32 ]
  %224 = phi <32 x float> [ %.lcssa8.lcssa37, %for_begin28.preheader ], [ %311, %for_body32 ]
  %225 = phi <32 x float> [ %.lcssa6.lcssa36, %for_begin28.preheader ], [ %310, %for_body32 ]
  %226 = phi <32 x float> [ %.lcssa4.lcssa34, %for_begin28.preheader ], [ %309, %for_body32 ]
  %227 = phi <32 x float> [ %.lcssa.lcssa32, %for_begin28.preheader ], [ %308, %for_body32 ]
  %228 = phi i32 [ 0, %for_begin28.preheader ], [ %321, %for_body32 ]
  %229 = mul nuw nsw i64 %indvars.iv, 58
  %230 = mul nuw nsw i32 %228, 58
  %231 = add nsw i64 %218, %229
  %232 = add nsw i32 %230, %219
  %233 = and i64 %231, 4294967294
  %234 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %233
  %235 = load float, float* %234, align 8, !tbaa !406
  %236 = insertelement <32 x float> undef, float %235, i32 0
  %237 = shufflevector <32 x float> %236, <32 x float> undef, <32 x i32> zeroinitializer
  %238 = shl nsw i64 %indvars.iv, 5
  %239 = add nuw nsw i64 %217, %238
  %240 = getelementptr inbounds [9216 x <32 x float>], [9216 x <32 x float>]* %5, i64 0, i64 0, i64 %239
  %241 = bitcast float* %240 to <32 x float>*
  %242 = load <32 x float>, <32 x float>* %241, align 16, !tbaa !412
  %243 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %237, <32 x float> %242, <32 x float> %227)
  %244 = or i32 %232, 1
  %245 = sext i32 %244 to i64
  %246 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %245
  %247 = load float, float* %246, align 4, !tbaa !406
  %248 = insertelement <32 x float> undef, float %247, i32 0
  %249 = shufflevector <32 x float> %248, <32 x float> undef, <32 x i32> zeroinitializer
  %250 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %249, <32 x float> %242, <32 x float> %226)
  %251 = add nuw nsw i64 %231, 2
  %252 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %251
  %253 = load float, float* %252, align 8, !tbaa !406
  %254 = insertelement <32 x float> undef, float %253, i32 0
  %255 = shufflevector <32 x float> %254, <32 x float> undef, <32 x i32> zeroinitializer
  %256 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %255, <32 x float> %242, <32 x float> %225)
  %257 = add nuw nsw i64 %231, 3
  %258 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %257
  %259 = load float, float* %258, align 4, !tbaa !406
  %260 = insertelement <32 x float> undef, float %259, i32 0
  %261 = shufflevector <32 x float> %260, <32 x float> undef, <32 x i32> zeroinitializer
  %262 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %261, <32 x float> %242, <32 x float> %224)
  %263 = add nuw nsw i64 %231, 4
  %264 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %263
  %265 = load float, float* %264, align 8, !tbaa !406
  %266 = insertelement <32 x float> undef, float %265, i32 0
  %267 = shufflevector <32 x float> %266, <32 x float> undef, <32 x i32> zeroinitializer
  %268 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %267, <32 x float> %242, <32 x float> %223)
  %269 = add nuw nsw i64 %231, 5
  %270 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %269
  %271 = load float, float* %270, align 4, !tbaa !406
  %272 = insertelement <32 x float> undef, float %271, i32 0
  %273 = shufflevector <32 x float> %272, <32 x float> undef, <32 x i32> zeroinitializer
  %274 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %273, <32 x float> %242, <32 x float> %222)
  %275 = add nuw nsw i64 %231, 6
  %276 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %275
  %277 = load float, float* %276, align 8, !tbaa !406
  %278 = insertelement <32 x float> undef, float %277, i32 0
  %279 = shufflevector <32 x float> %278, <32 x float> undef, <32 x i32> zeroinitializer
  %280 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %279, <32 x float> %242, <32 x float> %221)
  %281 = add nuw nsw i64 %231, 7
  %282 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %281
  %283 = load float, float* %282, align 4, !tbaa !406
  %284 = insertelement <32 x float> undef, float %283, i32 0
  %285 = shufflevector <32 x float> %284, <32 x float> undef, <32 x i32> zeroinitializer
  %286 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %285, <32 x float> %242, <32 x float> %220)
  %287 = add nuw nsw i64 %239, 1024
  %288 = getelementptr inbounds [9216 x <32 x float>], [9216 x <32 x float>]* %5, i64 0, i64 0, i64 %287
  %289 = bitcast float* %288 to <32 x float>*
  %290 = load <32 x float>, <32 x float>* %289, align 16, !tbaa !412
  %291 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %249, <32 x float> %290, <32 x float> %243)
  %292 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %255, <32 x float> %290, <32 x float> %250)
  %293 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %261, <32 x float> %290, <32 x float> %256)
  %294 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %267, <32 x float> %290, <32 x float> %262)
  %295 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %273, <32 x float> %290, <32 x float> %268)
  %296 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %279, <32 x float> %290, <32 x float> %274)
  %297 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %285, <32 x float> %290, <32 x float> %280)
  %298 = add nuw nsw i64 %231, 8
  %299 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %298
  %300 = load float, float* %299, align 8, !tbaa !406
  %301 = insertelement <32 x float> undef, float %300, i32 0
  %302 = shufflevector <32 x float> %301, <32 x float> undef, <32 x i32> zeroinitializer
  %303 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %302, <32 x float> %290, <32 x float> %286)
  %304 = add nuw nsw i64 %239, 2048
  %305 = getelementptr inbounds [9216 x <32 x float>], [9216 x <32 x float>]* %5, i64 0, i64 0, i64 %304
  %306 = bitcast float* %305 to <32 x float>*
  %307 = load <32 x float>, <32 x float>* %306, align 16, !tbaa !412
  %308 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %255, <32 x float> %307, <32 x float> %291)
  %309 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %261, <32 x float> %307, <32 x float> %292)
  %310 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %267, <32 x float> %307, <32 x float> %293)
  %311 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %273, <32 x float> %307, <32 x float> %294)
  %312 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %279, <32 x float> %307, <32 x float> %295)
  %313 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %285, <32 x float> %307, <32 x float> %296)
  %314 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %302, <32 x float> %307, <32 x float> %297)
  %315 = add nuw nsw i64 %231, 9
  %316 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %315
  %317 = load float, float* %316, align 4, !tbaa !406
  %318 = insertelement <32 x float> undef, float %317, i32 0
  %319 = shufflevector <32 x float> %318, <32 x float> undef, <32 x i32> zeroinitializer
  %320 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %319, <32 x float> %307, <32 x float> %303)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %321 = add nuw nsw i32 %228, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 32
  br i1 %exitcond, label %for_end33, label %for_body32, !prof !34

for_end33:                                        ; preds = %for_body32
  %322 = add nsw i64 %215, %206
  %323 = add nuw nsw i64 %217, 3072
  %324 = trunc i64 %322 to i32
  br label %for_body32.1

for_begin40.preheader:                            ; preds = %for_end42, %for_begin37.preheader
  %indvars.iv76 = phi i64 [ 0, %for_begin37.preheader ], [ %indvars.iv.next77, %for_end42 ]
  %325 = shl i64 %indvars.iv76, 3
  %326 = add nuw nsw i64 %325, %212
  %327 = shl i64 %indvars.iv76, 8
  br label %for_body41

for_end39:                                        ; preds = %for_end42
  %328 = add nuw nsw i32 %199, 1
  %exitcond79 = icmp eq i32 %328, 448
  br i1 %exitcond79, label %for_end21, label %for_begin22.preheader, !prof !34

for_body41:                                       ; preds = %for_body41, %for_begin40.preheader
  %indvars.iv73 = phi i64 [ 0, %for_begin40.preheader ], [ %indvars.iv.next74, %for_body41 ]
  %329 = add nuw nsw i64 %326, %indvars.iv73
  %330 = add nuw nsw i64 %329, 3136
  %331 = add nuw nsw i64 %329, 6272
  %332 = add nuw nsw i64 %329, 9408
  %333 = add nuw nsw i64 %329, 12544
  %334 = add nuw nsw i64 %329, 15680
  %335 = add nuw nsw i64 %329, 18816
  %336 = add nuw nsw i64 %329, 21952
  %337 = add nuw nsw i64 %329, 25088
  %338 = add nuw nsw i64 %329, 28224
  %339 = add nuw nsw i64 %329, 31360
  %340 = add nuw nsw i64 %329, 34496
  %341 = add nuw nsw i64 %329, 37632
  %342 = add nuw nsw i64 %329, 40768
  %343 = add nuw nsw i64 %329, 43904
  %344 = add nuw nsw i64 %329, 47040
  %345 = add nuw nsw i64 %329, 50176
  %346 = add nuw nsw i64 %329, 53312
  %347 = add nuw nsw i64 %329, 56448
  %348 = add nuw nsw i64 %329, 59584
  %349 = add nuw nsw i64 %329, 62720
  %350 = add nuw nsw i64 %329, 65856
  %351 = add nuw nsw i64 %329, 68992
  %352 = add nuw nsw i64 %329, 72128
  %353 = add nuw nsw i64 %329, 75264
  %354 = add nuw nsw i64 %329, 78400
  %355 = add nuw nsw i64 %329, 81536
  %356 = add nuw nsw i64 %329, 84672
  %357 = add nuw nsw i64 %329, 87808
  %358 = add nuw nsw i64 %329, 90944
  %359 = add nuw nsw i64 %329, 94080
  %360 = add nuw nsw i64 %329, 97216
  %361 = shl i64 %indvars.iv73, 5
  %362 = add nuw nsw i64 %361, %327
  %363 = getelementptr inbounds [56 x <32 x float>], [56 x <32 x float>]* %4, i64 0, i64 0, i64 %362
  %364 = bitcast float* %363 to <32 x float>*
  %365 = load <32 x float>, <32 x float>* %364, align 16, !tbaa !424
  %366 = getelementptr inbounds float, float* %56, i64 %329
  %367 = extractelement <32 x float> %365, i64 0
  store float %367, float* %366, align 4, !tbaa !427
  %368 = getelementptr inbounds float, float* %56, i64 %330
  %369 = extractelement <32 x float> %365, i64 1
  store float %369, float* %368, align 4, !tbaa !427
  %370 = getelementptr inbounds float, float* %56, i64 %331
  %371 = extractelement <32 x float> %365, i64 2
  store float %371, float* %370, align 4, !tbaa !427
  %372 = getelementptr inbounds float, float* %56, i64 %332
  %373 = extractelement <32 x float> %365, i64 3
  store float %373, float* %372, align 4, !tbaa !427
  %374 = getelementptr inbounds float, float* %56, i64 %333
  %375 = extractelement <32 x float> %365, i64 4
  store float %375, float* %374, align 4, !tbaa !427
  %376 = getelementptr inbounds float, float* %56, i64 %334
  %377 = extractelement <32 x float> %365, i64 5
  store float %377, float* %376, align 4, !tbaa !427
  %378 = getelementptr inbounds float, float* %56, i64 %335
  %379 = extractelement <32 x float> %365, i64 6
  store float %379, float* %378, align 4, !tbaa !427
  %380 = getelementptr inbounds float, float* %56, i64 %336
  %381 = extractelement <32 x float> %365, i64 7
  store float %381, float* %380, align 4, !tbaa !427
  %382 = getelementptr inbounds float, float* %56, i64 %337
  %383 = extractelement <32 x float> %365, i64 8
  store float %383, float* %382, align 4, !tbaa !427
  %384 = getelementptr inbounds float, float* %56, i64 %338
  %385 = extractelement <32 x float> %365, i64 9
  store float %385, float* %384, align 4, !tbaa !427
  %386 = getelementptr inbounds float, float* %56, i64 %339
  %387 = extractelement <32 x float> %365, i64 10
  store float %387, float* %386, align 4, !tbaa !427
  %388 = getelementptr inbounds float, float* %56, i64 %340
  %389 = extractelement <32 x float> %365, i64 11
  store float %389, float* %388, align 4, !tbaa !427
  %390 = getelementptr inbounds float, float* %56, i64 %341
  %391 = extractelement <32 x float> %365, i64 12
  store float %391, float* %390, align 4, !tbaa !427
  %392 = getelementptr inbounds float, float* %56, i64 %342
  %393 = extractelement <32 x float> %365, i64 13
  store float %393, float* %392, align 4, !tbaa !427
  %394 = getelementptr inbounds float, float* %56, i64 %343
  %395 = extractelement <32 x float> %365, i64 14
  store float %395, float* %394, align 4, !tbaa !427
  %396 = getelementptr inbounds float, float* %56, i64 %344
  %397 = extractelement <32 x float> %365, i64 15
  store float %397, float* %396, align 4, !tbaa !427
  %398 = getelementptr inbounds float, float* %56, i64 %345
  %399 = extractelement <32 x float> %365, i64 16
  store float %399, float* %398, align 4, !tbaa !427
  %400 = getelementptr inbounds float, float* %56, i64 %346
  %401 = extractelement <32 x float> %365, i64 17
  store float %401, float* %400, align 4, !tbaa !427
  %402 = getelementptr inbounds float, float* %56, i64 %347
  %403 = extractelement <32 x float> %365, i64 18
  store float %403, float* %402, align 4, !tbaa !427
  %404 = getelementptr inbounds float, float* %56, i64 %348
  %405 = extractelement <32 x float> %365, i64 19
  store float %405, float* %404, align 4, !tbaa !427
  %406 = getelementptr inbounds float, float* %56, i64 %349
  %407 = extractelement <32 x float> %365, i64 20
  store float %407, float* %406, align 4, !tbaa !427
  %408 = getelementptr inbounds float, float* %56, i64 %350
  %409 = extractelement <32 x float> %365, i64 21
  store float %409, float* %408, align 4, !tbaa !427
  %410 = getelementptr inbounds float, float* %56, i64 %351
  %411 = extractelement <32 x float> %365, i64 22
  store float %411, float* %410, align 4, !tbaa !427
  %412 = getelementptr inbounds float, float* %56, i64 %352
  %413 = extractelement <32 x float> %365, i64 23
  store float %413, float* %412, align 4, !tbaa !427
  %414 = getelementptr inbounds float, float* %56, i64 %353
  %415 = extractelement <32 x float> %365, i64 24
  store float %415, float* %414, align 4, !tbaa !427
  %416 = getelementptr inbounds float, float* %56, i64 %354
  %417 = extractelement <32 x float> %365, i64 25
  store float %417, float* %416, align 4, !tbaa !427
  %418 = getelementptr inbounds float, float* %56, i64 %355
  %419 = extractelement <32 x float> %365, i64 26
  store float %419, float* %418, align 4, !tbaa !427
  %420 = getelementptr inbounds float, float* %56, i64 %356
  %421 = extractelement <32 x float> %365, i64 27
  store float %421, float* %420, align 4, !tbaa !427
  %422 = getelementptr inbounds float, float* %56, i64 %357
  %423 = extractelement <32 x float> %365, i64 28
  store float %423, float* %422, align 4, !tbaa !427
  %424 = getelementptr inbounds float, float* %56, i64 %358
  %425 = extractelement <32 x float> %365, i64 29
  store float %425, float* %424, align 4, !tbaa !427
  %426 = getelementptr inbounds float, float* %56, i64 %359
  %427 = extractelement <32 x float> %365, i64 30
  store float %427, float* %426, align 4, !tbaa !427
  %428 = getelementptr inbounds float, float* %56, i64 %360
  %429 = extractelement <32 x float> %365, i64 31
  store float %429, float* %428, align 4, !tbaa !427
  %indvars.iv.next74 = add nuw nsw i64 %indvars.iv73, 1
  %exitcond75 = icmp eq i64 %indvars.iv.next74, 8
  br i1 %exitcond75, label %for_end42, label %for_body41, !prof !34

for_end42:                                        ; preds = %for_body41
  %indvars.iv.next77 = add nuw nsw i64 %indvars.iv76, 1
  %exitcond78 = icmp eq i64 %indvars.iv.next77, 7
  br i1 %exitcond78, label %for_end39, label %for_begin40.preheader, !prof !34

for_body32.1:                                     ; preds = %for_body32.1, %for_end33
  %indvars.iv.1 = phi i64 [ 0, %for_end33 ], [ %indvars.iv.next.1, %for_body32.1 ]
  %430 = phi <32 x float> [ %320, %for_end33 ], [ %530, %for_body32.1 ]
  %431 = phi <32 x float> [ %314, %for_end33 ], [ %524, %for_body32.1 ]
  %432 = phi <32 x float> [ %313, %for_end33 ], [ %523, %for_body32.1 ]
  %433 = phi <32 x float> [ %312, %for_end33 ], [ %522, %for_body32.1 ]
  %434 = phi <32 x float> [ %311, %for_end33 ], [ %521, %for_body32.1 ]
  %435 = phi <32 x float> [ %310, %for_end33 ], [ %520, %for_body32.1 ]
  %436 = phi <32 x float> [ %309, %for_end33 ], [ %519, %for_body32.1 ]
  %437 = phi <32 x float> [ %308, %for_end33 ], [ %518, %for_body32.1 ]
  %438 = phi i32 [ 0, %for_end33 ], [ %531, %for_body32.1 ]
  %439 = mul nuw nsw i64 %indvars.iv.1, 58
  %440 = mul nuw nsw i32 %438, 58
  %441 = add nsw i64 %322, %439
  %442 = add nsw i32 %440, %324
  %443 = and i64 %441, 4294967294
  %444 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %443
  %445 = load float, float* %444, align 8, !tbaa !406
  %446 = insertelement <32 x float> undef, float %445, i32 0
  %447 = shufflevector <32 x float> %446, <32 x float> undef, <32 x i32> zeroinitializer
  %448 = shl nsw i64 %indvars.iv.1, 5
  %449 = add nuw nsw i64 %323, %448
  %450 = getelementptr inbounds [9216 x <32 x float>], [9216 x <32 x float>]* %5, i64 0, i64 0, i64 %449
  %451 = bitcast float* %450 to <32 x float>*
  %452 = load <32 x float>, <32 x float>* %451, align 16, !tbaa !412
  %453 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %447, <32 x float> %452, <32 x float> %437)
  %454 = or i32 %442, 1
  %455 = sext i32 %454 to i64
  %456 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %455
  %457 = load float, float* %456, align 4, !tbaa !406
  %458 = insertelement <32 x float> undef, float %457, i32 0
  %459 = shufflevector <32 x float> %458, <32 x float> undef, <32 x i32> zeroinitializer
  %460 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %459, <32 x float> %452, <32 x float> %436)
  %461 = add nuw nsw i64 %441, 2
  %462 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %461
  %463 = load float, float* %462, align 8, !tbaa !406
  %464 = insertelement <32 x float> undef, float %463, i32 0
  %465 = shufflevector <32 x float> %464, <32 x float> undef, <32 x i32> zeroinitializer
  %466 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %465, <32 x float> %452, <32 x float> %435)
  %467 = add nuw nsw i64 %441, 3
  %468 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %467
  %469 = load float, float* %468, align 4, !tbaa !406
  %470 = insertelement <32 x float> undef, float %469, i32 0
  %471 = shufflevector <32 x float> %470, <32 x float> undef, <32 x i32> zeroinitializer
  %472 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %471, <32 x float> %452, <32 x float> %434)
  %473 = add nuw nsw i64 %441, 4
  %474 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %473
  %475 = load float, float* %474, align 8, !tbaa !406
  %476 = insertelement <32 x float> undef, float %475, i32 0
  %477 = shufflevector <32 x float> %476, <32 x float> undef, <32 x i32> zeroinitializer
  %478 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %477, <32 x float> %452, <32 x float> %433)
  %479 = add nuw nsw i64 %441, 5
  %480 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %479
  %481 = load float, float* %480, align 4, !tbaa !406
  %482 = insertelement <32 x float> undef, float %481, i32 0
  %483 = shufflevector <32 x float> %482, <32 x float> undef, <32 x i32> zeroinitializer
  %484 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %483, <32 x float> %452, <32 x float> %432)
  %485 = add nuw nsw i64 %441, 6
  %486 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %485
  %487 = load float, float* %486, align 8, !tbaa !406
  %488 = insertelement <32 x float> undef, float %487, i32 0
  %489 = shufflevector <32 x float> %488, <32 x float> undef, <32 x i32> zeroinitializer
  %490 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %489, <32 x float> %452, <32 x float> %431)
  %491 = add nuw nsw i64 %441, 7
  %492 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %491
  %493 = load float, float* %492, align 4, !tbaa !406
  %494 = insertelement <32 x float> undef, float %493, i32 0
  %495 = shufflevector <32 x float> %494, <32 x float> undef, <32 x i32> zeroinitializer
  %496 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %495, <32 x float> %452, <32 x float> %430)
  %497 = add nuw nsw i64 %449, 1024
  %498 = getelementptr inbounds [9216 x <32 x float>], [9216 x <32 x float>]* %5, i64 0, i64 0, i64 %497
  %499 = bitcast float* %498 to <32 x float>*
  %500 = load <32 x float>, <32 x float>* %499, align 16, !tbaa !412
  %501 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %459, <32 x float> %500, <32 x float> %453)
  %502 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %465, <32 x float> %500, <32 x float> %460)
  %503 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %471, <32 x float> %500, <32 x float> %466)
  %504 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %477, <32 x float> %500, <32 x float> %472)
  %505 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %483, <32 x float> %500, <32 x float> %478)
  %506 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %489, <32 x float> %500, <32 x float> %484)
  %507 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %495, <32 x float> %500, <32 x float> %490)
  %508 = add nuw nsw i64 %441, 8
  %509 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %508
  %510 = load float, float* %509, align 8, !tbaa !406
  %511 = insertelement <32 x float> undef, float %510, i32 0
  %512 = shufflevector <32 x float> %511, <32 x float> undef, <32 x i32> zeroinitializer
  %513 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %512, <32 x float> %500, <32 x float> %496)
  %514 = add nuw nsw i64 %449, 2048
  %515 = getelementptr inbounds [9216 x <32 x float>], [9216 x <32 x float>]* %5, i64 0, i64 0, i64 %514
  %516 = bitcast float* %515 to <32 x float>*
  %517 = load <32 x float>, <32 x float>* %516, align 16, !tbaa !412
  %518 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %465, <32 x float> %517, <32 x float> %501)
  %519 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %471, <32 x float> %517, <32 x float> %502)
  %520 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %477, <32 x float> %517, <32 x float> %503)
  %521 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %483, <32 x float> %517, <32 x float> %504)
  %522 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %489, <32 x float> %517, <32 x float> %505)
  %523 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %495, <32 x float> %517, <32 x float> %506)
  %524 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %512, <32 x float> %517, <32 x float> %507)
  %525 = add nuw nsw i64 %441, 9
  %526 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %525
  %527 = load float, float* %526, align 4, !tbaa !406
  %528 = insertelement <32 x float> undef, float %527, i32 0
  %529 = shufflevector <32 x float> %528, <32 x float> undef, <32 x i32> zeroinitializer
  %530 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %529, <32 x float> %517, <32 x float> %513)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %531 = add nuw nsw i32 %438, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 32
  br i1 %exitcond.1, label %for_end33.1, label %for_body32.1, !prof !34

for_end33.1:                                      ; preds = %for_body32.1
  %532 = add nsw i64 %215, %208
  %533 = add nuw nsw i64 %217, 6144
  %534 = trunc i64 %532 to i32
  br label %for_body32.2

for_body32.2:                                     ; preds = %for_body32.2, %for_end33.1
  %indvars.iv.2 = phi i64 [ 0, %for_end33.1 ], [ %indvars.iv.next.2, %for_body32.2 ]
  %535 = phi <32 x float> [ %530, %for_end33.1 ], [ %635, %for_body32.2 ]
  %536 = phi <32 x float> [ %524, %for_end33.1 ], [ %629, %for_body32.2 ]
  %537 = phi <32 x float> [ %523, %for_end33.1 ], [ %628, %for_body32.2 ]
  %538 = phi <32 x float> [ %522, %for_end33.1 ], [ %627, %for_body32.2 ]
  %539 = phi <32 x float> [ %521, %for_end33.1 ], [ %626, %for_body32.2 ]
  %540 = phi <32 x float> [ %520, %for_end33.1 ], [ %625, %for_body32.2 ]
  %541 = phi <32 x float> [ %519, %for_end33.1 ], [ %624, %for_body32.2 ]
  %542 = phi <32 x float> [ %518, %for_end33.1 ], [ %623, %for_body32.2 ]
  %543 = phi i32 [ 0, %for_end33.1 ], [ %636, %for_body32.2 ]
  %544 = mul nuw nsw i64 %indvars.iv.2, 58
  %545 = mul nuw nsw i32 %543, 58
  %546 = add nsw i64 %532, %544
  %547 = add nsw i32 %545, %534
  %548 = and i64 %546, 4294967294
  %549 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %548
  %550 = load float, float* %549, align 8, !tbaa !406
  %551 = insertelement <32 x float> undef, float %550, i32 0
  %552 = shufflevector <32 x float> %551, <32 x float> undef, <32 x i32> zeroinitializer
  %553 = shl nsw i64 %indvars.iv.2, 5
  %554 = add nuw nsw i64 %533, %553
  %555 = getelementptr inbounds [9216 x <32 x float>], [9216 x <32 x float>]* %5, i64 0, i64 0, i64 %554
  %556 = bitcast float* %555 to <32 x float>*
  %557 = load <32 x float>, <32 x float>* %556, align 16, !tbaa !412
  %558 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %552, <32 x float> %557, <32 x float> %542)
  %559 = or i32 %547, 1
  %560 = sext i32 %559 to i64
  %561 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %560
  %562 = load float, float* %561, align 4, !tbaa !406
  %563 = insertelement <32 x float> undef, float %562, i32 0
  %564 = shufflevector <32 x float> %563, <32 x float> undef, <32 x i32> zeroinitializer
  %565 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %564, <32 x float> %557, <32 x float> %541)
  %566 = add nuw nsw i64 %546, 2
  %567 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %566
  %568 = load float, float* %567, align 8, !tbaa !406
  %569 = insertelement <32 x float> undef, float %568, i32 0
  %570 = shufflevector <32 x float> %569, <32 x float> undef, <32 x i32> zeroinitializer
  %571 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %570, <32 x float> %557, <32 x float> %540)
  %572 = add nuw nsw i64 %546, 3
  %573 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %572
  %574 = load float, float* %573, align 4, !tbaa !406
  %575 = insertelement <32 x float> undef, float %574, i32 0
  %576 = shufflevector <32 x float> %575, <32 x float> undef, <32 x i32> zeroinitializer
  %577 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %576, <32 x float> %557, <32 x float> %539)
  %578 = add nuw nsw i64 %546, 4
  %579 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %578
  %580 = load float, float* %579, align 8, !tbaa !406
  %581 = insertelement <32 x float> undef, float %580, i32 0
  %582 = shufflevector <32 x float> %581, <32 x float> undef, <32 x i32> zeroinitializer
  %583 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %582, <32 x float> %557, <32 x float> %538)
  %584 = add nuw nsw i64 %546, 5
  %585 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %584
  %586 = load float, float* %585, align 4, !tbaa !406
  %587 = insertelement <32 x float> undef, float %586, i32 0
  %588 = shufflevector <32 x float> %587, <32 x float> undef, <32 x i32> zeroinitializer
  %589 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %588, <32 x float> %557, <32 x float> %537)
  %590 = add nuw nsw i64 %546, 6
  %591 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %590
  %592 = load float, float* %591, align 8, !tbaa !406
  %593 = insertelement <32 x float> undef, float %592, i32 0
  %594 = shufflevector <32 x float> %593, <32 x float> undef, <32 x i32> zeroinitializer
  %595 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %594, <32 x float> %557, <32 x float> %536)
  %596 = add nuw nsw i64 %546, 7
  %597 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %596
  %598 = load float, float* %597, align 4, !tbaa !406
  %599 = insertelement <32 x float> undef, float %598, i32 0
  %600 = shufflevector <32 x float> %599, <32 x float> undef, <32 x i32> zeroinitializer
  %601 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %600, <32 x float> %557, <32 x float> %535)
  %602 = add nuw nsw i64 %554, 1024
  %603 = getelementptr inbounds [9216 x <32 x float>], [9216 x <32 x float>]* %5, i64 0, i64 0, i64 %602
  %604 = bitcast float* %603 to <32 x float>*
  %605 = load <32 x float>, <32 x float>* %604, align 16, !tbaa !412
  %606 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %564, <32 x float> %605, <32 x float> %558)
  %607 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %570, <32 x float> %605, <32 x float> %565)
  %608 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %576, <32 x float> %605, <32 x float> %571)
  %609 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %582, <32 x float> %605, <32 x float> %577)
  %610 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %588, <32 x float> %605, <32 x float> %583)
  %611 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %594, <32 x float> %605, <32 x float> %589)
  %612 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %600, <32 x float> %605, <32 x float> %595)
  %613 = add nuw nsw i64 %546, 8
  %614 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %613
  %615 = load float, float* %614, align 8, !tbaa !406
  %616 = insertelement <32 x float> undef, float %615, i32 0
  %617 = shufflevector <32 x float> %616, <32 x float> undef, <32 x i32> zeroinitializer
  %618 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %617, <32 x float> %605, <32 x float> %601)
  %619 = add nuw nsw i64 %554, 2048
  %620 = getelementptr inbounds [9216 x <32 x float>], [9216 x <32 x float>]* %5, i64 0, i64 0, i64 %619
  %621 = bitcast float* %620 to <32 x float>*
  %622 = load <32 x float>, <32 x float>* %621, align 16, !tbaa !412
  %623 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %570, <32 x float> %622, <32 x float> %606)
  %624 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %576, <32 x float> %622, <32 x float> %607)
  %625 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %582, <32 x float> %622, <32 x float> %608)
  %626 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %588, <32 x float> %622, <32 x float> %609)
  %627 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %594, <32 x float> %622, <32 x float> %610)
  %628 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %600, <32 x float> %622, <32 x float> %611)
  %629 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %617, <32 x float> %622, <32 x float> %612)
  %630 = add nuw nsw i64 %546, 9
  %631 = getelementptr inbounds [430592 x float], [430592 x float]* %6, i64 0, i64 %630
  %632 = load float, float* %631, align 4, !tbaa !406
  %633 = insertelement <32 x float> undef, float %632, i32 0
  %634 = shufflevector <32 x float> %633, <32 x float> undef, <32 x i32> zeroinitializer
  %635 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %634, <32 x float> %622, <32 x float> %618)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %636 = add nuw nsw i32 %543, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 32
  br i1 %exitcond.2, label %for_end33.2, label %for_body32.2, !prof !34

for_end33.2:                                      ; preds = %for_body32.2
  %indvars.iv.next66 = add nuw nsw i64 %indvars.iv65, 1
  %exitcond67 = icmp eq i64 %indvars.iv.next66, 4
  br i1 %exitcond67, label %for_begin34.preheader, label %for_begin28.preheader, !prof !34
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_conv2d_6(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
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
  tail call fastcc void @fused_nn_conv2d_6_compute_(i8* %12, i8* %14, i8* %16)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_conv2d_6_compute_(i8* noalias nocapture readonly, i8* noalias readonly, i8* noalias) unnamed_addr #3 {
entry:
  %3 = alloca [4 x <64 x float>], align 16
  %4 = alloca [112 x <64 x float>], align 16
  %5 = alloca [1152 x <64 x float>], align 16
  %6 = alloca [831744 x float], align 16
  %7 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar65 = phi i64 [ 0, %entry ], [ %indvar.next66, %for_end3 ]
  %8 = mul nuw nsw i64 %indvar65, 3648
  %9 = trunc i64 %indvar65 to i32
  %10 = urem i32 %9, 114
  %11 = udiv i32 %9, 114
  %.off = add nsw i32 %10, -1
  %12 = icmp ult i32 %.off, 112
  br i1 %12, label %for_begin4.preheader.us.preheader, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep72 = getelementptr [831744 x float], [831744 x float]* %6, i64 0, i64 %8
  %scevgep7273 = bitcast float* %scevgep72 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 16 %scevgep7273, i8 0, i64 14592, i1 false)
  br label %for_end3

for_begin4.preheader.us.preheader:                ; preds = %for_begin1.preheader
  %13 = mul nsw i32 %11, 401408
  %14 = add nsw i32 %13, -113
  %15 = mul nuw nsw i32 %10, 112
  %16 = add i32 %14, %15
  %17 = sext i32 %16 to i64
  br label %for_begin4.preheader.us

for_begin4.preheader.us:                          ; preds = %for_end6.us-lcssa.us.us, %for_begin4.preheader.us.preheader
  %indvars.iv77 = phi i64 [ 0, %for_begin4.preheader.us.preheader ], [ %indvars.iv.next78, %for_end6.us-lcssa.us.us ]
  %18 = mul nuw nsw i64 %indvars.iv77, 114
  %19 = add nuw nsw i64 %18, %8
  %20 = mul nuw nsw i64 %indvars.iv77, 12544
  %21 = add nsw i64 %20, %17
  br label %for_body5.us.us

for_body5.us.us:                                  ; preds = %if_end.us.us, %for_begin4.preheader.us
  %indvars.iv74 = phi i64 [ 0, %for_begin4.preheader.us ], [ %indvars.iv.next75, %if_end.us.us ]
  %22 = phi i32 [ 0, %for_begin4.preheader.us ], [ %29, %if_end.us.us ]
  %23 = add nuw nsw i64 %19, %indvars.iv74
  %trunc.us.us = trunc i32 %22 to i31
  switch i31 %trunc.us.us, label %if_then.us.us [
    i31 113, label %if_end.us.us
    i31 0, label %if_end.us.us
  ]

if_then.us.us:                                    ; preds = %for_body5.us.us
  %24 = add nsw i64 %21, %indvars.iv74
  %25 = getelementptr inbounds float, float* %7, i64 %24
  %26 = load float, float* %25, align 4, !tbaa !430
  br label %if_end.us.us

if_end.us.us:                                     ; preds = %if_then.us.us, %for_body5.us.us, %for_body5.us.us
  %27 = phi float [ %26, %if_then.us.us ], [ 0.000000e+00, %for_body5.us.us ], [ 0.000000e+00, %for_body5.us.us ]
  %28 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %23
  store float %27, float* %28, align 4, !tbaa !433
  %indvars.iv.next75 = add nuw nsw i64 %indvars.iv74, 1
  %29 = add nuw nsw i32 %22, 1
  %exitcond76 = icmp eq i64 %indvars.iv.next75, 114
  br i1 %exitcond76, label %for_end6.us-lcssa.us.us, label %for_body5.us.us, !prof !34

for_end6.us-lcssa.us.us:                          ; preds = %if_end.us.us
  %indvars.iv.next78 = add nuw nsw i64 %indvars.iv77, 1
  %exitcond79 = icmp eq i64 %indvars.iv.next78, 32
  br i1 %exitcond79, label %for_end3, label %for_begin4.preheader.us, !prof !34

for_begin7.preheader:                             ; preds = %for_end3
  %30 = bitcast [4 x <64 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [4 x <64 x float>], [4 x <64 x float>]* %3, i64 0, i64 0
  %31 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_end3:                                         ; preds = %for_end6.us-lcssa.us.us, %for_begin4.preheader.preheader
  %indvar.next66 = add nuw nsw i64 %indvar65, 1
  %exitcond80 = icmp eq i64 %indvar.next66, 228
  br i1 %exitcond80, label %for_begin7.preheader, label %for_begin1.preheader, !prof !34

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %32 = phi i32 [ 0, %for_begin7.preheader ], [ %54, %for_end12 ]
  %33 = urem i32 %32, 3
  %34 = mul nuw nsw i32 %33, 6144
  %35 = udiv i32 %32, 3
  %36 = mul nsw i32 %35, 36864
  %37 = add nuw i32 %34, %36
  %38 = mul nuw nsw i32 %33, 3
  %39 = or i32 %38, %36
  %40 = zext i32 %39 to i64
  %41 = sext i32 %37 to i64
  br label %for_begin13.preheader

for_begin19.preheader:                            ; preds = %for_end12
  %42 = getelementptr inbounds [4 x <64 x float>], [4 x <64 x float>]* %3, i64 0, i64 0, i64 64
  %43 = bitcast float* %42 to <64 x float>*
  %44 = getelementptr inbounds [4 x <64 x float>], [4 x <64 x float>]* %3, i64 0, i64 0, i64 128
  %45 = bitcast float* %44 to <64 x float>*
  %46 = getelementptr inbounds [4 x <64 x float>], [4 x <64 x float>]* %3, i64 0, i64 0, i64 192
  %47 = bitcast float* %46 to <64 x float>*
  %48 = bitcast i8* %2 to float*
  %49 = bitcast [4 x <64 x float>]* %3 to i8*
  br label %for_begin22.preheader

for_begin13.preheader:                            ; preds = %for_end15, %for_begin10.preheader
  %indvars.iv58 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next59, %for_end15 ]
  %50 = mul nuw nsw i64 %indvars.iv58, 18432
  %51 = add nuw nsw i64 %50, %41
  %52 = mul nuw nsw i64 %indvars.iv58, 288
  %53 = add nuw nsw i64 %52, %40
  br label %for_begin16.preheader

for_end12:                                        ; preds = %for_end15
  %54 = add nuw nsw i32 %32, 1
  %exitcond61 = icmp eq i32 %54, 6
  br i1 %exitcond61, label %for_begin19.preheader, label %for_begin10.preheader, !prof !34

for_begin16.preheader:                            ; preds = %for_end18, %for_begin13.preheader
  %indvars.iv55 = phi i64 [ 0, %for_begin13.preheader ], [ %indvars.iv.next56, %for_end18 ]
  %55 = shl i64 %indvars.iv55, 11
  %56 = add nuw nsw i64 %51, %55
  %57 = add nuw nsw i64 %53, %indvars.iv55
  br label %for_body17

for_end15:                                        ; preds = %for_end18
  %indvars.iv.next59 = add nuw nsw i64 %indvars.iv58, 1
  %exitcond60 = icmp eq i64 %indvars.iv.next59, 2
  br i1 %exitcond60, label %for_end12, label %for_begin13.preheader, !prof !34

for_body17:                                       ; preds = %for_body17, %for_begin16.preheader
  %indvars.iv52 = phi i64 [ 0, %for_begin16.preheader ], [ %indvars.iv.next53, %for_body17 ]
  %58 = shl i64 %indvars.iv52, 6
  %59 = add nuw nsw i64 %56, %58
  %60 = mul nuw nsw i64 %indvars.iv52, 9
  %61 = add nuw nsw i64 %57, %60
  %62 = add nuw nsw i64 %61, 576
  %63 = add nuw nsw i64 %61, 1152
  %64 = add nuw nsw i64 %61, 1728
  %65 = add nuw nsw i64 %61, 2304
  %66 = add nuw nsw i64 %61, 2880
  %67 = add nuw nsw i64 %61, 3456
  %68 = add nuw nsw i64 %61, 4032
  %69 = add nuw nsw i64 %61, 4608
  %70 = add nuw nsw i64 %61, 5184
  %71 = add nuw nsw i64 %61, 5760
  %72 = add nuw nsw i64 %61, 6336
  %73 = add nuw nsw i64 %61, 6912
  %74 = add nuw nsw i64 %61, 7488
  %75 = add nuw nsw i64 %61, 8064
  %76 = add nuw nsw i64 %61, 8640
  %77 = add nuw nsw i64 %61, 9216
  %78 = add nuw nsw i64 %61, 9792
  %79 = add nuw nsw i64 %61, 10368
  %80 = add nuw nsw i64 %61, 10944
  %81 = add nuw nsw i64 %61, 11520
  %82 = add nuw nsw i64 %61, 12096
  %83 = add nuw nsw i64 %61, 12672
  %84 = add nuw nsw i64 %61, 13248
  %85 = add nuw nsw i64 %61, 13824
  %86 = add nuw nsw i64 %61, 14400
  %87 = add nuw nsw i64 %61, 14976
  %88 = add nuw nsw i64 %61, 15552
  %89 = add nuw nsw i64 %61, 16128
  %90 = add nuw nsw i64 %61, 16704
  %91 = add nuw nsw i64 %61, 17280
  %92 = add nuw nsw i64 %61, 17856
  %93 = add nuw nsw i64 %61, 18432
  %94 = add nuw nsw i64 %61, 19008
  %95 = add nuw nsw i64 %61, 19584
  %96 = add nuw nsw i64 %61, 20160
  %97 = add nuw nsw i64 %61, 20736
  %98 = add nuw nsw i64 %61, 21312
  %99 = add nuw nsw i64 %61, 21888
  %100 = add nuw nsw i64 %61, 22464
  %101 = add nuw nsw i64 %61, 23040
  %102 = add nuw nsw i64 %61, 23616
  %103 = add nuw nsw i64 %61, 24192
  %104 = add nuw nsw i64 %61, 24768
  %105 = add nuw nsw i64 %61, 25344
  %106 = add nuw nsw i64 %61, 25920
  %107 = add nuw nsw i64 %61, 26496
  %108 = add nuw nsw i64 %61, 27072
  %109 = add nuw nsw i64 %61, 27648
  %110 = add nuw nsw i64 %61, 28224
  %111 = add nuw nsw i64 %61, 28800
  %112 = add nuw nsw i64 %61, 29376
  %113 = add nuw nsw i64 %61, 29952
  %114 = add nuw nsw i64 %61, 30528
  %115 = add nuw nsw i64 %61, 31104
  %116 = add nuw nsw i64 %61, 31680
  %117 = add nuw nsw i64 %61, 32256
  %118 = add nuw nsw i64 %61, 32832
  %119 = add nuw nsw i64 %61, 33408
  %120 = add nuw nsw i64 %61, 33984
  %121 = add nuw nsw i64 %61, 34560
  %122 = add nuw nsw i64 %61, 35136
  %123 = add nuw nsw i64 %61, 35712
  %124 = add nuw nsw i64 %61, 36288
  %125 = getelementptr inbounds float, float* %31, i64 %61
  %126 = load float, float* %125, align 4, !tbaa !436
  %127 = insertelement <64 x float> undef, float %126, i32 0
  %128 = getelementptr inbounds float, float* %31, i64 %62
  %129 = load float, float* %128, align 4, !tbaa !436
  %130 = insertelement <64 x float> %127, float %129, i32 1
  %131 = getelementptr inbounds float, float* %31, i64 %63
  %132 = load float, float* %131, align 4, !tbaa !436
  %133 = insertelement <64 x float> %130, float %132, i32 2
  %134 = getelementptr inbounds float, float* %31, i64 %64
  %135 = load float, float* %134, align 4, !tbaa !436
  %136 = insertelement <64 x float> %133, float %135, i32 3
  %137 = getelementptr inbounds float, float* %31, i64 %65
  %138 = load float, float* %137, align 4, !tbaa !436
  %139 = insertelement <64 x float> %136, float %138, i32 4
  %140 = getelementptr inbounds float, float* %31, i64 %66
  %141 = load float, float* %140, align 4, !tbaa !436
  %142 = insertelement <64 x float> %139, float %141, i32 5
  %143 = getelementptr inbounds float, float* %31, i64 %67
  %144 = load float, float* %143, align 4, !tbaa !436
  %145 = insertelement <64 x float> %142, float %144, i32 6
  %146 = getelementptr inbounds float, float* %31, i64 %68
  %147 = load float, float* %146, align 4, !tbaa !436
  %148 = insertelement <64 x float> %145, float %147, i32 7
  %149 = getelementptr inbounds float, float* %31, i64 %69
  %150 = load float, float* %149, align 4, !tbaa !436
  %151 = insertelement <64 x float> %148, float %150, i32 8
  %152 = getelementptr inbounds float, float* %31, i64 %70
  %153 = load float, float* %152, align 4, !tbaa !436
  %154 = insertelement <64 x float> %151, float %153, i32 9
  %155 = getelementptr inbounds float, float* %31, i64 %71
  %156 = load float, float* %155, align 4, !tbaa !436
  %157 = insertelement <64 x float> %154, float %156, i32 10
  %158 = getelementptr inbounds float, float* %31, i64 %72
  %159 = load float, float* %158, align 4, !tbaa !436
  %160 = insertelement <64 x float> %157, float %159, i32 11
  %161 = getelementptr inbounds float, float* %31, i64 %73
  %162 = load float, float* %161, align 4, !tbaa !436
  %163 = insertelement <64 x float> %160, float %162, i32 12
  %164 = getelementptr inbounds float, float* %31, i64 %74
  %165 = load float, float* %164, align 4, !tbaa !436
  %166 = insertelement <64 x float> %163, float %165, i32 13
  %167 = getelementptr inbounds float, float* %31, i64 %75
  %168 = load float, float* %167, align 4, !tbaa !436
  %169 = insertelement <64 x float> %166, float %168, i32 14
  %170 = getelementptr inbounds float, float* %31, i64 %76
  %171 = load float, float* %170, align 4, !tbaa !436
  %172 = insertelement <64 x float> %169, float %171, i32 15
  %173 = getelementptr inbounds float, float* %31, i64 %77
  %174 = load float, float* %173, align 4, !tbaa !436
  %175 = insertelement <64 x float> %172, float %174, i32 16
  %176 = getelementptr inbounds float, float* %31, i64 %78
  %177 = load float, float* %176, align 4, !tbaa !436
  %178 = insertelement <64 x float> %175, float %177, i32 17
  %179 = getelementptr inbounds float, float* %31, i64 %79
  %180 = load float, float* %179, align 4, !tbaa !436
  %181 = insertelement <64 x float> %178, float %180, i32 18
  %182 = getelementptr inbounds float, float* %31, i64 %80
  %183 = load float, float* %182, align 4, !tbaa !436
  %184 = insertelement <64 x float> %181, float %183, i32 19
  %185 = getelementptr inbounds float, float* %31, i64 %81
  %186 = load float, float* %185, align 4, !tbaa !436
  %187 = insertelement <64 x float> %184, float %186, i32 20
  %188 = getelementptr inbounds float, float* %31, i64 %82
  %189 = load float, float* %188, align 4, !tbaa !436
  %190 = insertelement <64 x float> %187, float %189, i32 21
  %191 = getelementptr inbounds float, float* %31, i64 %83
  %192 = load float, float* %191, align 4, !tbaa !436
  %193 = insertelement <64 x float> %190, float %192, i32 22
  %194 = getelementptr inbounds float, float* %31, i64 %84
  %195 = load float, float* %194, align 4, !tbaa !436
  %196 = insertelement <64 x float> %193, float %195, i32 23
  %197 = getelementptr inbounds float, float* %31, i64 %85
  %198 = load float, float* %197, align 4, !tbaa !436
  %199 = insertelement <64 x float> %196, float %198, i32 24
  %200 = getelementptr inbounds float, float* %31, i64 %86
  %201 = load float, float* %200, align 4, !tbaa !436
  %202 = insertelement <64 x float> %199, float %201, i32 25
  %203 = getelementptr inbounds float, float* %31, i64 %87
  %204 = load float, float* %203, align 4, !tbaa !436
  %205 = insertelement <64 x float> %202, float %204, i32 26
  %206 = getelementptr inbounds float, float* %31, i64 %88
  %207 = load float, float* %206, align 4, !tbaa !436
  %208 = insertelement <64 x float> %205, float %207, i32 27
  %209 = getelementptr inbounds float, float* %31, i64 %89
  %210 = load float, float* %209, align 4, !tbaa !436
  %211 = insertelement <64 x float> %208, float %210, i32 28
  %212 = getelementptr inbounds float, float* %31, i64 %90
  %213 = load float, float* %212, align 4, !tbaa !436
  %214 = insertelement <64 x float> %211, float %213, i32 29
  %215 = getelementptr inbounds float, float* %31, i64 %91
  %216 = load float, float* %215, align 4, !tbaa !436
  %217 = insertelement <64 x float> %214, float %216, i32 30
  %218 = getelementptr inbounds float, float* %31, i64 %92
  %219 = load float, float* %218, align 4, !tbaa !436
  %220 = insertelement <64 x float> %217, float %219, i32 31
  %221 = getelementptr inbounds float, float* %31, i64 %93
  %222 = load float, float* %221, align 4, !tbaa !436
  %223 = insertelement <64 x float> %220, float %222, i32 32
  %224 = getelementptr inbounds float, float* %31, i64 %94
  %225 = load float, float* %224, align 4, !tbaa !436
  %226 = insertelement <64 x float> %223, float %225, i32 33
  %227 = getelementptr inbounds float, float* %31, i64 %95
  %228 = load float, float* %227, align 4, !tbaa !436
  %229 = insertelement <64 x float> %226, float %228, i32 34
  %230 = getelementptr inbounds float, float* %31, i64 %96
  %231 = load float, float* %230, align 4, !tbaa !436
  %232 = insertelement <64 x float> %229, float %231, i32 35
  %233 = getelementptr inbounds float, float* %31, i64 %97
  %234 = load float, float* %233, align 4, !tbaa !436
  %235 = insertelement <64 x float> %232, float %234, i32 36
  %236 = getelementptr inbounds float, float* %31, i64 %98
  %237 = load float, float* %236, align 4, !tbaa !436
  %238 = insertelement <64 x float> %235, float %237, i32 37
  %239 = getelementptr inbounds float, float* %31, i64 %99
  %240 = load float, float* %239, align 4, !tbaa !436
  %241 = insertelement <64 x float> %238, float %240, i32 38
  %242 = getelementptr inbounds float, float* %31, i64 %100
  %243 = load float, float* %242, align 4, !tbaa !436
  %244 = insertelement <64 x float> %241, float %243, i32 39
  %245 = getelementptr inbounds float, float* %31, i64 %101
  %246 = load float, float* %245, align 4, !tbaa !436
  %247 = insertelement <64 x float> %244, float %246, i32 40
  %248 = getelementptr inbounds float, float* %31, i64 %102
  %249 = load float, float* %248, align 4, !tbaa !436
  %250 = insertelement <64 x float> %247, float %249, i32 41
  %251 = getelementptr inbounds float, float* %31, i64 %103
  %252 = load float, float* %251, align 4, !tbaa !436
  %253 = insertelement <64 x float> %250, float %252, i32 42
  %254 = getelementptr inbounds float, float* %31, i64 %104
  %255 = load float, float* %254, align 4, !tbaa !436
  %256 = insertelement <64 x float> %253, float %255, i32 43
  %257 = getelementptr inbounds float, float* %31, i64 %105
  %258 = load float, float* %257, align 4, !tbaa !436
  %259 = insertelement <64 x float> %256, float %258, i32 44
  %260 = getelementptr inbounds float, float* %31, i64 %106
  %261 = load float, float* %260, align 4, !tbaa !436
  %262 = insertelement <64 x float> %259, float %261, i32 45
  %263 = getelementptr inbounds float, float* %31, i64 %107
  %264 = load float, float* %263, align 4, !tbaa !436
  %265 = insertelement <64 x float> %262, float %264, i32 46
  %266 = getelementptr inbounds float, float* %31, i64 %108
  %267 = load float, float* %266, align 4, !tbaa !436
  %268 = insertelement <64 x float> %265, float %267, i32 47
  %269 = getelementptr inbounds float, float* %31, i64 %109
  %270 = load float, float* %269, align 4, !tbaa !436
  %271 = insertelement <64 x float> %268, float %270, i32 48
  %272 = getelementptr inbounds float, float* %31, i64 %110
  %273 = load float, float* %272, align 4, !tbaa !436
  %274 = insertelement <64 x float> %271, float %273, i32 49
  %275 = getelementptr inbounds float, float* %31, i64 %111
  %276 = load float, float* %275, align 4, !tbaa !436
  %277 = insertelement <64 x float> %274, float %276, i32 50
  %278 = getelementptr inbounds float, float* %31, i64 %112
  %279 = load float, float* %278, align 4, !tbaa !436
  %280 = insertelement <64 x float> %277, float %279, i32 51
  %281 = getelementptr inbounds float, float* %31, i64 %113
  %282 = load float, float* %281, align 4, !tbaa !436
  %283 = insertelement <64 x float> %280, float %282, i32 52
  %284 = getelementptr inbounds float, float* %31, i64 %114
  %285 = load float, float* %284, align 4, !tbaa !436
  %286 = insertelement <64 x float> %283, float %285, i32 53
  %287 = getelementptr inbounds float, float* %31, i64 %115
  %288 = load float, float* %287, align 4, !tbaa !436
  %289 = insertelement <64 x float> %286, float %288, i32 54
  %290 = getelementptr inbounds float, float* %31, i64 %116
  %291 = load float, float* %290, align 4, !tbaa !436
  %292 = insertelement <64 x float> %289, float %291, i32 55
  %293 = getelementptr inbounds float, float* %31, i64 %117
  %294 = load float, float* %293, align 4, !tbaa !436
  %295 = insertelement <64 x float> %292, float %294, i32 56
  %296 = getelementptr inbounds float, float* %31, i64 %118
  %297 = load float, float* %296, align 4, !tbaa !436
  %298 = insertelement <64 x float> %295, float %297, i32 57
  %299 = getelementptr inbounds float, float* %31, i64 %119
  %300 = load float, float* %299, align 4, !tbaa !436
  %301 = insertelement <64 x float> %298, float %300, i32 58
  %302 = getelementptr inbounds float, float* %31, i64 %120
  %303 = load float, float* %302, align 4, !tbaa !436
  %304 = insertelement <64 x float> %301, float %303, i32 59
  %305 = getelementptr inbounds float, float* %31, i64 %121
  %306 = load float, float* %305, align 4, !tbaa !436
  %307 = insertelement <64 x float> %304, float %306, i32 60
  %308 = getelementptr inbounds float, float* %31, i64 %122
  %309 = load float, float* %308, align 4, !tbaa !436
  %310 = insertelement <64 x float> %307, float %309, i32 61
  %311 = getelementptr inbounds float, float* %31, i64 %123
  %312 = load float, float* %311, align 4, !tbaa !436
  %313 = insertelement <64 x float> %310, float %312, i32 62
  %314 = getelementptr inbounds float, float* %31, i64 %124
  %315 = load float, float* %314, align 4, !tbaa !436
  %316 = insertelement <64 x float> %313, float %315, i32 63
  %317 = getelementptr inbounds [1152 x <64 x float>], [1152 x <64 x float>]* %5, i64 0, i64 0, i64 %59
  %318 = bitcast float* %317 to <64 x float>*
  store <64 x float> %316, <64 x float>* %318, align 16, !tbaa !439
  %indvars.iv.next53 = add nuw nsw i64 %indvars.iv52, 1
  %exitcond54 = icmp eq i64 %indvars.iv.next53, 32
  br i1 %exitcond54, label %for_end18, label %for_body17, !prof !34

for_end18:                                        ; preds = %for_body17
  %indvars.iv.next56 = add nuw nsw i64 %indvars.iv55, 1
  %exitcond57 = icmp eq i64 %indvars.iv.next56, 3
  br i1 %exitcond57, label %for_end15, label %for_begin16.preheader, !prof !34

for_begin22.preheader:                            ; preds = %for_end39, %for_begin19.preheader
  %319 = phi i32 [ 0, %for_begin19.preheader ], [ %412, %for_end39 ]
  %320 = urem i32 %319, 112
  %321 = udiv i32 %319, 112
  %322 = mul nsw i32 %321, 36864
  %323 = zext i32 %322 to i64
  %reass.mul = mul nuw nsw i32 %320, 3648
  %324 = zext i32 %reass.mul to i64
  %325 = mul nuw nsw i32 %320, 3648
  %reass.mul.1 = add nuw nsw i32 %325, 3648
  %326 = zext i32 %reass.mul.1 to i64
  %327 = mul nuw nsw i32 %320, 3648
  %reass.mul.2 = add nuw nsw i32 %327, 7296
  %328 = zext i32 %reass.mul.2 to i64
  br label %for_body23

for_end21:                                        ; preds = %for_end39
  ret void

for_begin37.preheader:                            ; preds = %for_begin34.preheader
  %329 = mul nuw nsw i32 %320, 112
  %330 = mul nsw i32 %321, 802816
  %331 = or i32 %330, %329
  br label %for_begin40.preheader

for_body23:                                       ; preds = %for_begin34.preheader, %for_begin22.preheader
  %indvar = phi i64 [ 0, %for_begin22.preheader ], [ %indvar.next, %for_begin34.preheader ]
  %332 = shl i64 %indvar, 2
  %scevgep = getelementptr [112 x <64 x float>], [112 x <64 x float>]* %4, i64 0, i64 %332
  %scevgep43 = bitcast <64 x float>* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %49, i8 0, i64 1024, i1 false)
  br label %for_begin28.preheader

for_begin34.preheader:                            ; preds = %for_end33.2
  store <64 x float> %735, <64 x float>* %.sub, align 16, !tbaa !442
  store <64 x float> %736, <64 x float>* %43, align 16, !tbaa !442
  store <64 x float> %737, <64 x float>* %45, align 16, !tbaa !442
  store <64 x float> %743, <64 x float>* %47, align 16, !tbaa !442
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep43, i8* nonnull align 16 %30, i64 1024, i1 false)
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond44 = icmp eq i64 %indvar.next, 28
  br i1 %exitcond44, label %for_begin37.preheader, label %for_body23, !prof !34

for_begin28.preheader:                            ; preds = %for_end33.2, %for_body23
  %indvars.iv37 = phi i64 [ 0, %for_body23 ], [ %indvars.iv.next38, %for_end33.2 ]
  %.lcssa8.lcssa21 = phi <64 x float> [ zeroinitializer, %for_body23 ], [ %743, %for_end33.2 ]
  %.lcssa6.lcssa20 = phi <64 x float> [ zeroinitializer, %for_body23 ], [ %737, %for_end33.2 ]
  %.lcssa4.lcssa18 = phi <64 x float> [ zeroinitializer, %for_body23 ], [ %736, %for_end33.2 ]
  %.lcssa.lcssa16 = phi <64 x float> [ zeroinitializer, %for_body23 ], [ %735, %for_end33.2 ]
  %333 = mul nuw nsw i64 %indvars.iv37, 415872
  %334 = add nuw nsw i64 %333, %332
  %335 = mul nuw nsw i64 %indvars.iv37, 18432
  %336 = add nuw nsw i64 %335, %323
  %337 = add nsw i64 %334, %324
  %338 = trunc i64 %337 to i32
  br label %for_body32

for_body32:                                       ; preds = %for_body32, %for_begin28.preheader
  %indvars.iv = phi i64 [ 0, %for_begin28.preheader ], [ %indvars.iv.next, %for_body32 ]
  %339 = phi <64 x float> [ %.lcssa8.lcssa21, %for_begin28.preheader ], [ %403, %for_body32 ]
  %340 = phi <64 x float> [ %.lcssa6.lcssa20, %for_begin28.preheader ], [ %397, %for_body32 ]
  %341 = phi <64 x float> [ %.lcssa4.lcssa18, %for_begin28.preheader ], [ %396, %for_body32 ]
  %342 = phi <64 x float> [ %.lcssa.lcssa16, %for_begin28.preheader ], [ %395, %for_body32 ]
  %343 = phi i32 [ 0, %for_begin28.preheader ], [ %404, %for_body32 ]
  %344 = mul nuw nsw i64 %indvars.iv, 114
  %345 = mul nuw nsw i32 %343, 114
  %346 = add nsw i64 %337, %344
  %347 = add nsw i32 %345, %338
  %348 = and i64 %346, 4294967294
  %349 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %348
  %350 = load float, float* %349, align 8, !tbaa !433
  %351 = insertelement <64 x float> undef, float %350, i32 0
  %352 = shufflevector <64 x float> %351, <64 x float> undef, <64 x i32> zeroinitializer
  %353 = shl nsw i64 %indvars.iv, 6
  %354 = add nuw nsw i64 %336, %353
  %355 = getelementptr inbounds [1152 x <64 x float>], [1152 x <64 x float>]* %5, i64 0, i64 0, i64 %354
  %356 = bitcast float* %355 to <64 x float>*
  %357 = load <64 x float>, <64 x float>* %356, align 16, !tbaa !439
  %358 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %352, <64 x float> %357, <64 x float> %342)
  %359 = or i32 %347, 1
  %360 = sext i32 %359 to i64
  %361 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %360
  %362 = load float, float* %361, align 4, !tbaa !433
  %363 = insertelement <64 x float> undef, float %362, i32 0
  %364 = shufflevector <64 x float> %363, <64 x float> undef, <64 x i32> zeroinitializer
  %365 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %364, <64 x float> %357, <64 x float> %341)
  %366 = add nuw nsw i64 %346, 2
  %367 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %366
  %368 = load float, float* %367, align 8, !tbaa !433
  %369 = insertelement <64 x float> undef, float %368, i32 0
  %370 = shufflevector <64 x float> %369, <64 x float> undef, <64 x i32> zeroinitializer
  %371 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %370, <64 x float> %357, <64 x float> %340)
  %372 = add nuw nsw i64 %346, 3
  %373 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %372
  %374 = load float, float* %373, align 4, !tbaa !433
  %375 = insertelement <64 x float> undef, float %374, i32 0
  %376 = shufflevector <64 x float> %375, <64 x float> undef, <64 x i32> zeroinitializer
  %377 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %376, <64 x float> %357, <64 x float> %339)
  %378 = add nuw nsw i64 %354, 2048
  %379 = getelementptr inbounds [1152 x <64 x float>], [1152 x <64 x float>]* %5, i64 0, i64 0, i64 %378
  %380 = bitcast float* %379 to <64 x float>*
  %381 = load <64 x float>, <64 x float>* %380, align 16, !tbaa !439
  %382 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %364, <64 x float> %381, <64 x float> %358)
  %383 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %370, <64 x float> %381, <64 x float> %365)
  %384 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %376, <64 x float> %381, <64 x float> %371)
  %385 = add nuw nsw i64 %346, 4
  %386 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %385
  %387 = load float, float* %386, align 8, !tbaa !433
  %388 = insertelement <64 x float> undef, float %387, i32 0
  %389 = shufflevector <64 x float> %388, <64 x float> undef, <64 x i32> zeroinitializer
  %390 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %389, <64 x float> %381, <64 x float> %377)
  %391 = add nuw nsw i64 %354, 4096
  %392 = getelementptr inbounds [1152 x <64 x float>], [1152 x <64 x float>]* %5, i64 0, i64 0, i64 %391
  %393 = bitcast float* %392 to <64 x float>*
  %394 = load <64 x float>, <64 x float>* %393, align 16, !tbaa !439
  %395 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %370, <64 x float> %394, <64 x float> %382)
  %396 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %376, <64 x float> %394, <64 x float> %383)
  %397 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %389, <64 x float> %394, <64 x float> %384)
  %398 = add nuw nsw i64 %346, 5
  %399 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %398
  %400 = load float, float* %399, align 4, !tbaa !433
  %401 = insertelement <64 x float> undef, float %400, i32 0
  %402 = shufflevector <64 x float> %401, <64 x float> undef, <64 x i32> zeroinitializer
  %403 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %402, <64 x float> %394, <64 x float> %390)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %404 = add nuw nsw i32 %343, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 32
  br i1 %exitcond, label %for_end33, label %for_body32, !prof !34

for_end33:                                        ; preds = %for_body32
  %405 = add nsw i64 %334, %326
  %406 = add nuw nsw i64 %336, 6144
  %407 = trunc i64 %405 to i32
  br label %for_body32.1

for_begin40.preheader:                            ; preds = %for_end42, %for_begin37.preheader
  %indvars.iv48 = phi i64 [ 0, %for_begin37.preheader ], [ %indvars.iv.next49, %for_end42 ]
  %408 = shl i64 %indvars.iv48, 8
  %indvars.iv48.tr = trunc i64 %indvars.iv48 to i32
  %409 = shl i32 %indvars.iv48.tr, 2
  %410 = add i32 %331, %409
  %411 = zext i32 %410 to i64
  br label %for_body41

for_end39:                                        ; preds = %for_end42
  %412 = add nuw nsw i32 %319, 1
  %exitcond51 = icmp eq i32 %412, 224
  br i1 %exitcond51, label %for_end21, label %for_begin22.preheader, !prof !34

for_body41:                                       ; preds = %for_body41, %for_begin40.preheader
  %indvars.iv45 = phi i64 [ 0, %for_begin40.preheader ], [ %indvars.iv.next46, %for_body41 ]
  %413 = add nuw nsw i64 %indvars.iv45, %411
  %414 = add nuw nsw i64 %413, 12544
  %415 = add nuw nsw i64 %413, 25088
  %416 = add nuw nsw i64 %413, 37632
  %417 = add nuw nsw i64 %413, 50176
  %418 = add nuw nsw i64 %413, 62720
  %419 = add nuw nsw i64 %413, 75264
  %420 = add nuw nsw i64 %413, 87808
  %421 = add nuw nsw i64 %413, 100352
  %422 = add nuw nsw i64 %413, 112896
  %423 = add nuw nsw i64 %413, 125440
  %424 = add nuw nsw i64 %413, 137984
  %425 = add nuw nsw i64 %413, 150528
  %426 = add nuw nsw i64 %413, 163072
  %427 = add nuw nsw i64 %413, 175616
  %428 = add nuw nsw i64 %413, 188160
  %429 = add nuw nsw i64 %413, 200704
  %430 = add nuw nsw i64 %413, 213248
  %431 = add nuw nsw i64 %413, 225792
  %432 = add nuw nsw i64 %413, 238336
  %433 = add nuw nsw i64 %413, 250880
  %434 = add nuw nsw i64 %413, 263424
  %435 = add nuw nsw i64 %413, 275968
  %436 = add nuw nsw i64 %413, 288512
  %437 = add nuw nsw i64 %413, 301056
  %438 = add nuw nsw i64 %413, 313600
  %439 = add nuw nsw i64 %413, 326144
  %440 = add nuw nsw i64 %413, 338688
  %441 = add nuw nsw i64 %413, 351232
  %442 = add nuw nsw i64 %413, 363776
  %443 = add nuw nsw i64 %413, 376320
  %444 = add nuw nsw i64 %413, 388864
  %445 = add nuw nsw i64 %413, 401408
  %446 = add nuw nsw i64 %413, 413952
  %447 = add nuw nsw i64 %413, 426496
  %448 = add nuw nsw i64 %413, 439040
  %449 = add nuw nsw i64 %413, 451584
  %450 = add nuw nsw i64 %413, 464128
  %451 = add nuw nsw i64 %413, 476672
  %452 = add nuw nsw i64 %413, 489216
  %453 = add nuw nsw i64 %413, 501760
  %454 = add nuw nsw i64 %413, 514304
  %455 = add nuw nsw i64 %413, 526848
  %456 = add nuw nsw i64 %413, 539392
  %457 = add nuw nsw i64 %413, 551936
  %458 = add nuw nsw i64 %413, 564480
  %459 = add nuw nsw i64 %413, 577024
  %460 = add nuw nsw i64 %413, 589568
  %461 = add nuw nsw i64 %413, 602112
  %462 = add nuw nsw i64 %413, 614656
  %463 = add nuw nsw i64 %413, 627200
  %464 = add nuw nsw i64 %413, 639744
  %465 = add nuw nsw i64 %413, 652288
  %466 = add nuw nsw i64 %413, 664832
  %467 = add nuw nsw i64 %413, 677376
  %468 = add nuw nsw i64 %413, 689920
  %469 = add nuw nsw i64 %413, 702464
  %470 = add nuw nsw i64 %413, 715008
  %471 = add nuw nsw i64 %413, 727552
  %472 = add nuw nsw i64 %413, 740096
  %473 = add nuw nsw i64 %413, 752640
  %474 = add nuw nsw i64 %413, 765184
  %475 = add nuw nsw i64 %413, 777728
  %476 = add nuw nsw i64 %413, 790272
  %477 = shl i64 %indvars.iv45, 6
  %478 = add nuw nsw i64 %477, %408
  %479 = getelementptr inbounds [112 x <64 x float>], [112 x <64 x float>]* %4, i64 0, i64 0, i64 %478
  %480 = bitcast float* %479 to <64 x float>*
  %481 = load <64 x float>, <64 x float>* %480, align 16, !tbaa !450
  %482 = getelementptr inbounds float, float* %48, i64 %413
  %483 = extractelement <64 x float> %481, i64 0
  store float %483, float* %482, align 4, !tbaa !453
  %484 = getelementptr inbounds float, float* %48, i64 %414
  %485 = extractelement <64 x float> %481, i64 1
  store float %485, float* %484, align 4, !tbaa !453
  %486 = getelementptr inbounds float, float* %48, i64 %415
  %487 = extractelement <64 x float> %481, i64 2
  store float %487, float* %486, align 4, !tbaa !453
  %488 = getelementptr inbounds float, float* %48, i64 %416
  %489 = extractelement <64 x float> %481, i64 3
  store float %489, float* %488, align 4, !tbaa !453
  %490 = getelementptr inbounds float, float* %48, i64 %417
  %491 = extractelement <64 x float> %481, i64 4
  store float %491, float* %490, align 4, !tbaa !453
  %492 = getelementptr inbounds float, float* %48, i64 %418
  %493 = extractelement <64 x float> %481, i64 5
  store float %493, float* %492, align 4, !tbaa !453
  %494 = getelementptr inbounds float, float* %48, i64 %419
  %495 = extractelement <64 x float> %481, i64 6
  store float %495, float* %494, align 4, !tbaa !453
  %496 = getelementptr inbounds float, float* %48, i64 %420
  %497 = extractelement <64 x float> %481, i64 7
  store float %497, float* %496, align 4, !tbaa !453
  %498 = getelementptr inbounds float, float* %48, i64 %421
  %499 = extractelement <64 x float> %481, i64 8
  store float %499, float* %498, align 4, !tbaa !453
  %500 = getelementptr inbounds float, float* %48, i64 %422
  %501 = extractelement <64 x float> %481, i64 9
  store float %501, float* %500, align 4, !tbaa !453
  %502 = getelementptr inbounds float, float* %48, i64 %423
  %503 = extractelement <64 x float> %481, i64 10
  store float %503, float* %502, align 4, !tbaa !453
  %504 = getelementptr inbounds float, float* %48, i64 %424
  %505 = extractelement <64 x float> %481, i64 11
  store float %505, float* %504, align 4, !tbaa !453
  %506 = getelementptr inbounds float, float* %48, i64 %425
  %507 = extractelement <64 x float> %481, i64 12
  store float %507, float* %506, align 4, !tbaa !453
  %508 = getelementptr inbounds float, float* %48, i64 %426
  %509 = extractelement <64 x float> %481, i64 13
  store float %509, float* %508, align 4, !tbaa !453
  %510 = getelementptr inbounds float, float* %48, i64 %427
  %511 = extractelement <64 x float> %481, i64 14
  store float %511, float* %510, align 4, !tbaa !453
  %512 = getelementptr inbounds float, float* %48, i64 %428
  %513 = extractelement <64 x float> %481, i64 15
  store float %513, float* %512, align 4, !tbaa !453
  %514 = getelementptr inbounds float, float* %48, i64 %429
  %515 = extractelement <64 x float> %481, i64 16
  store float %515, float* %514, align 4, !tbaa !453
  %516 = getelementptr inbounds float, float* %48, i64 %430
  %517 = extractelement <64 x float> %481, i64 17
  store float %517, float* %516, align 4, !tbaa !453
  %518 = getelementptr inbounds float, float* %48, i64 %431
  %519 = extractelement <64 x float> %481, i64 18
  store float %519, float* %518, align 4, !tbaa !453
  %520 = getelementptr inbounds float, float* %48, i64 %432
  %521 = extractelement <64 x float> %481, i64 19
  store float %521, float* %520, align 4, !tbaa !453
  %522 = getelementptr inbounds float, float* %48, i64 %433
  %523 = extractelement <64 x float> %481, i64 20
  store float %523, float* %522, align 4, !tbaa !453
  %524 = getelementptr inbounds float, float* %48, i64 %434
  %525 = extractelement <64 x float> %481, i64 21
  store float %525, float* %524, align 4, !tbaa !453
  %526 = getelementptr inbounds float, float* %48, i64 %435
  %527 = extractelement <64 x float> %481, i64 22
  store float %527, float* %526, align 4, !tbaa !453
  %528 = getelementptr inbounds float, float* %48, i64 %436
  %529 = extractelement <64 x float> %481, i64 23
  store float %529, float* %528, align 4, !tbaa !453
  %530 = getelementptr inbounds float, float* %48, i64 %437
  %531 = extractelement <64 x float> %481, i64 24
  store float %531, float* %530, align 4, !tbaa !453
  %532 = getelementptr inbounds float, float* %48, i64 %438
  %533 = extractelement <64 x float> %481, i64 25
  store float %533, float* %532, align 4, !tbaa !453
  %534 = getelementptr inbounds float, float* %48, i64 %439
  %535 = extractelement <64 x float> %481, i64 26
  store float %535, float* %534, align 4, !tbaa !453
  %536 = getelementptr inbounds float, float* %48, i64 %440
  %537 = extractelement <64 x float> %481, i64 27
  store float %537, float* %536, align 4, !tbaa !453
  %538 = getelementptr inbounds float, float* %48, i64 %441
  %539 = extractelement <64 x float> %481, i64 28
  store float %539, float* %538, align 4, !tbaa !453
  %540 = getelementptr inbounds float, float* %48, i64 %442
  %541 = extractelement <64 x float> %481, i64 29
  store float %541, float* %540, align 4, !tbaa !453
  %542 = getelementptr inbounds float, float* %48, i64 %443
  %543 = extractelement <64 x float> %481, i64 30
  store float %543, float* %542, align 4, !tbaa !453
  %544 = getelementptr inbounds float, float* %48, i64 %444
  %545 = extractelement <64 x float> %481, i64 31
  store float %545, float* %544, align 4, !tbaa !453
  %546 = getelementptr inbounds float, float* %48, i64 %445
  %547 = extractelement <64 x float> %481, i64 32
  store float %547, float* %546, align 4, !tbaa !453
  %548 = getelementptr inbounds float, float* %48, i64 %446
  %549 = extractelement <64 x float> %481, i64 33
  store float %549, float* %548, align 4, !tbaa !453
  %550 = getelementptr inbounds float, float* %48, i64 %447
  %551 = extractelement <64 x float> %481, i64 34
  store float %551, float* %550, align 4, !tbaa !453
  %552 = getelementptr inbounds float, float* %48, i64 %448
  %553 = extractelement <64 x float> %481, i64 35
  store float %553, float* %552, align 4, !tbaa !453
  %554 = getelementptr inbounds float, float* %48, i64 %449
  %555 = extractelement <64 x float> %481, i64 36
  store float %555, float* %554, align 4, !tbaa !453
  %556 = getelementptr inbounds float, float* %48, i64 %450
  %557 = extractelement <64 x float> %481, i64 37
  store float %557, float* %556, align 4, !tbaa !453
  %558 = getelementptr inbounds float, float* %48, i64 %451
  %559 = extractelement <64 x float> %481, i64 38
  store float %559, float* %558, align 4, !tbaa !453
  %560 = getelementptr inbounds float, float* %48, i64 %452
  %561 = extractelement <64 x float> %481, i64 39
  store float %561, float* %560, align 4, !tbaa !453
  %562 = getelementptr inbounds float, float* %48, i64 %453
  %563 = extractelement <64 x float> %481, i64 40
  store float %563, float* %562, align 4, !tbaa !453
  %564 = getelementptr inbounds float, float* %48, i64 %454
  %565 = extractelement <64 x float> %481, i64 41
  store float %565, float* %564, align 4, !tbaa !453
  %566 = getelementptr inbounds float, float* %48, i64 %455
  %567 = extractelement <64 x float> %481, i64 42
  store float %567, float* %566, align 4, !tbaa !453
  %568 = getelementptr inbounds float, float* %48, i64 %456
  %569 = extractelement <64 x float> %481, i64 43
  store float %569, float* %568, align 4, !tbaa !453
  %570 = getelementptr inbounds float, float* %48, i64 %457
  %571 = extractelement <64 x float> %481, i64 44
  store float %571, float* %570, align 4, !tbaa !453
  %572 = getelementptr inbounds float, float* %48, i64 %458
  %573 = extractelement <64 x float> %481, i64 45
  store float %573, float* %572, align 4, !tbaa !453
  %574 = getelementptr inbounds float, float* %48, i64 %459
  %575 = extractelement <64 x float> %481, i64 46
  store float %575, float* %574, align 4, !tbaa !453
  %576 = getelementptr inbounds float, float* %48, i64 %460
  %577 = extractelement <64 x float> %481, i64 47
  store float %577, float* %576, align 4, !tbaa !453
  %578 = getelementptr inbounds float, float* %48, i64 %461
  %579 = extractelement <64 x float> %481, i64 48
  store float %579, float* %578, align 4, !tbaa !453
  %580 = getelementptr inbounds float, float* %48, i64 %462
  %581 = extractelement <64 x float> %481, i64 49
  store float %581, float* %580, align 4, !tbaa !453
  %582 = getelementptr inbounds float, float* %48, i64 %463
  %583 = extractelement <64 x float> %481, i64 50
  store float %583, float* %582, align 4, !tbaa !453
  %584 = getelementptr inbounds float, float* %48, i64 %464
  %585 = extractelement <64 x float> %481, i64 51
  store float %585, float* %584, align 4, !tbaa !453
  %586 = getelementptr inbounds float, float* %48, i64 %465
  %587 = extractelement <64 x float> %481, i64 52
  store float %587, float* %586, align 4, !tbaa !453
  %588 = getelementptr inbounds float, float* %48, i64 %466
  %589 = extractelement <64 x float> %481, i64 53
  store float %589, float* %588, align 4, !tbaa !453
  %590 = getelementptr inbounds float, float* %48, i64 %467
  %591 = extractelement <64 x float> %481, i64 54
  store float %591, float* %590, align 4, !tbaa !453
  %592 = getelementptr inbounds float, float* %48, i64 %468
  %593 = extractelement <64 x float> %481, i64 55
  store float %593, float* %592, align 4, !tbaa !453
  %594 = getelementptr inbounds float, float* %48, i64 %469
  %595 = extractelement <64 x float> %481, i64 56
  store float %595, float* %594, align 4, !tbaa !453
  %596 = getelementptr inbounds float, float* %48, i64 %470
  %597 = extractelement <64 x float> %481, i64 57
  store float %597, float* %596, align 4, !tbaa !453
  %598 = getelementptr inbounds float, float* %48, i64 %471
  %599 = extractelement <64 x float> %481, i64 58
  store float %599, float* %598, align 4, !tbaa !453
  %600 = getelementptr inbounds float, float* %48, i64 %472
  %601 = extractelement <64 x float> %481, i64 59
  store float %601, float* %600, align 4, !tbaa !453
  %602 = getelementptr inbounds float, float* %48, i64 %473
  %603 = extractelement <64 x float> %481, i64 60
  store float %603, float* %602, align 4, !tbaa !453
  %604 = getelementptr inbounds float, float* %48, i64 %474
  %605 = extractelement <64 x float> %481, i64 61
  store float %605, float* %604, align 4, !tbaa !453
  %606 = getelementptr inbounds float, float* %48, i64 %475
  %607 = extractelement <64 x float> %481, i64 62
  store float %607, float* %606, align 4, !tbaa !453
  %608 = getelementptr inbounds float, float* %48, i64 %476
  %609 = extractelement <64 x float> %481, i64 63
  store float %609, float* %608, align 4, !tbaa !453
  %indvars.iv.next46 = add nuw nsw i64 %indvars.iv45, 1
  %exitcond47 = icmp eq i64 %indvars.iv.next46, 4
  br i1 %exitcond47, label %for_end42, label %for_body41, !prof !34

for_end42:                                        ; preds = %for_body41
  %indvars.iv.next49 = add nuw nsw i64 %indvars.iv48, 1
  %exitcond50 = icmp eq i64 %indvars.iv.next49, 28
  br i1 %exitcond50, label %for_end39, label %for_begin40.preheader, !prof !34

for_body32.1:                                     ; preds = %for_body32.1, %for_end33
  %indvars.iv.1 = phi i64 [ 0, %for_end33 ], [ %indvars.iv.next.1, %for_body32.1 ]
  %610 = phi <64 x float> [ %403, %for_end33 ], [ %674, %for_body32.1 ]
  %611 = phi <64 x float> [ %397, %for_end33 ], [ %668, %for_body32.1 ]
  %612 = phi <64 x float> [ %396, %for_end33 ], [ %667, %for_body32.1 ]
  %613 = phi <64 x float> [ %395, %for_end33 ], [ %666, %for_body32.1 ]
  %614 = phi i32 [ 0, %for_end33 ], [ %675, %for_body32.1 ]
  %615 = mul nuw nsw i64 %indvars.iv.1, 114
  %616 = mul nuw nsw i32 %614, 114
  %617 = add nsw i64 %405, %615
  %618 = add nsw i32 %616, %407
  %619 = and i64 %617, 4294967294
  %620 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %619
  %621 = load float, float* %620, align 8, !tbaa !433
  %622 = insertelement <64 x float> undef, float %621, i32 0
  %623 = shufflevector <64 x float> %622, <64 x float> undef, <64 x i32> zeroinitializer
  %624 = shl nsw i64 %indvars.iv.1, 6
  %625 = add nuw nsw i64 %406, %624
  %626 = getelementptr inbounds [1152 x <64 x float>], [1152 x <64 x float>]* %5, i64 0, i64 0, i64 %625
  %627 = bitcast float* %626 to <64 x float>*
  %628 = load <64 x float>, <64 x float>* %627, align 16, !tbaa !439
  %629 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %623, <64 x float> %628, <64 x float> %613)
  %630 = or i32 %618, 1
  %631 = sext i32 %630 to i64
  %632 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %631
  %633 = load float, float* %632, align 4, !tbaa !433
  %634 = insertelement <64 x float> undef, float %633, i32 0
  %635 = shufflevector <64 x float> %634, <64 x float> undef, <64 x i32> zeroinitializer
  %636 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %635, <64 x float> %628, <64 x float> %612)
  %637 = add nuw nsw i64 %617, 2
  %638 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %637
  %639 = load float, float* %638, align 8, !tbaa !433
  %640 = insertelement <64 x float> undef, float %639, i32 0
  %641 = shufflevector <64 x float> %640, <64 x float> undef, <64 x i32> zeroinitializer
  %642 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %641, <64 x float> %628, <64 x float> %611)
  %643 = add nuw nsw i64 %617, 3
  %644 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %643
  %645 = load float, float* %644, align 4, !tbaa !433
  %646 = insertelement <64 x float> undef, float %645, i32 0
  %647 = shufflevector <64 x float> %646, <64 x float> undef, <64 x i32> zeroinitializer
  %648 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %647, <64 x float> %628, <64 x float> %610)
  %649 = add nuw nsw i64 %625, 2048
  %650 = getelementptr inbounds [1152 x <64 x float>], [1152 x <64 x float>]* %5, i64 0, i64 0, i64 %649
  %651 = bitcast float* %650 to <64 x float>*
  %652 = load <64 x float>, <64 x float>* %651, align 16, !tbaa !439
  %653 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %635, <64 x float> %652, <64 x float> %629)
  %654 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %641, <64 x float> %652, <64 x float> %636)
  %655 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %647, <64 x float> %652, <64 x float> %642)
  %656 = add nuw nsw i64 %617, 4
  %657 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %656
  %658 = load float, float* %657, align 8, !tbaa !433
  %659 = insertelement <64 x float> undef, float %658, i32 0
  %660 = shufflevector <64 x float> %659, <64 x float> undef, <64 x i32> zeroinitializer
  %661 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %660, <64 x float> %652, <64 x float> %648)
  %662 = add nuw nsw i64 %625, 4096
  %663 = getelementptr inbounds [1152 x <64 x float>], [1152 x <64 x float>]* %5, i64 0, i64 0, i64 %662
  %664 = bitcast float* %663 to <64 x float>*
  %665 = load <64 x float>, <64 x float>* %664, align 16, !tbaa !439
  %666 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %641, <64 x float> %665, <64 x float> %653)
  %667 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %647, <64 x float> %665, <64 x float> %654)
  %668 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %660, <64 x float> %665, <64 x float> %655)
  %669 = add nuw nsw i64 %617, 5
  %670 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %669
  %671 = load float, float* %670, align 4, !tbaa !433
  %672 = insertelement <64 x float> undef, float %671, i32 0
  %673 = shufflevector <64 x float> %672, <64 x float> undef, <64 x i32> zeroinitializer
  %674 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %673, <64 x float> %665, <64 x float> %661)
  %indvars.iv.next.1 = add nuw nsw i64 %indvars.iv.1, 1
  %675 = add nuw nsw i32 %614, 1
  %exitcond.1 = icmp eq i64 %indvars.iv.next.1, 32
  br i1 %exitcond.1, label %for_end33.1, label %for_body32.1, !prof !34

for_end33.1:                                      ; preds = %for_body32.1
  %676 = add nsw i64 %334, %328
  %677 = add nuw nsw i64 %336, 12288
  %678 = trunc i64 %676 to i32
  br label %for_body32.2

for_body32.2:                                     ; preds = %for_body32.2, %for_end33.1
  %indvars.iv.2 = phi i64 [ 0, %for_end33.1 ], [ %indvars.iv.next.2, %for_body32.2 ]
  %679 = phi <64 x float> [ %674, %for_end33.1 ], [ %743, %for_body32.2 ]
  %680 = phi <64 x float> [ %668, %for_end33.1 ], [ %737, %for_body32.2 ]
  %681 = phi <64 x float> [ %667, %for_end33.1 ], [ %736, %for_body32.2 ]
  %682 = phi <64 x float> [ %666, %for_end33.1 ], [ %735, %for_body32.2 ]
  %683 = phi i32 [ 0, %for_end33.1 ], [ %744, %for_body32.2 ]
  %684 = mul nuw nsw i64 %indvars.iv.2, 114
  %685 = mul nuw nsw i32 %683, 114
  %686 = add nsw i64 %676, %684
  %687 = add nsw i32 %685, %678
  %688 = and i64 %686, 4294967294
  %689 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %688
  %690 = load float, float* %689, align 8, !tbaa !433
  %691 = insertelement <64 x float> undef, float %690, i32 0
  %692 = shufflevector <64 x float> %691, <64 x float> undef, <64 x i32> zeroinitializer
  %693 = shl nsw i64 %indvars.iv.2, 6
  %694 = add nuw nsw i64 %677, %693
  %695 = getelementptr inbounds [1152 x <64 x float>], [1152 x <64 x float>]* %5, i64 0, i64 0, i64 %694
  %696 = bitcast float* %695 to <64 x float>*
  %697 = load <64 x float>, <64 x float>* %696, align 16, !tbaa !439
  %698 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %692, <64 x float> %697, <64 x float> %682)
  %699 = or i32 %687, 1
  %700 = sext i32 %699 to i64
  %701 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %700
  %702 = load float, float* %701, align 4, !tbaa !433
  %703 = insertelement <64 x float> undef, float %702, i32 0
  %704 = shufflevector <64 x float> %703, <64 x float> undef, <64 x i32> zeroinitializer
  %705 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %704, <64 x float> %697, <64 x float> %681)
  %706 = add nuw nsw i64 %686, 2
  %707 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %706
  %708 = load float, float* %707, align 8, !tbaa !433
  %709 = insertelement <64 x float> undef, float %708, i32 0
  %710 = shufflevector <64 x float> %709, <64 x float> undef, <64 x i32> zeroinitializer
  %711 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %710, <64 x float> %697, <64 x float> %680)
  %712 = add nuw nsw i64 %686, 3
  %713 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %712
  %714 = load float, float* %713, align 4, !tbaa !433
  %715 = insertelement <64 x float> undef, float %714, i32 0
  %716 = shufflevector <64 x float> %715, <64 x float> undef, <64 x i32> zeroinitializer
  %717 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %716, <64 x float> %697, <64 x float> %679)
  %718 = add nuw nsw i64 %694, 2048
  %719 = getelementptr inbounds [1152 x <64 x float>], [1152 x <64 x float>]* %5, i64 0, i64 0, i64 %718
  %720 = bitcast float* %719 to <64 x float>*
  %721 = load <64 x float>, <64 x float>* %720, align 16, !tbaa !439
  %722 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %704, <64 x float> %721, <64 x float> %698)
  %723 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %710, <64 x float> %721, <64 x float> %705)
  %724 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %716, <64 x float> %721, <64 x float> %711)
  %725 = add nuw nsw i64 %686, 4
  %726 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %725
  %727 = load float, float* %726, align 8, !tbaa !433
  %728 = insertelement <64 x float> undef, float %727, i32 0
  %729 = shufflevector <64 x float> %728, <64 x float> undef, <64 x i32> zeroinitializer
  %730 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %729, <64 x float> %721, <64 x float> %717)
  %731 = add nuw nsw i64 %694, 4096
  %732 = getelementptr inbounds [1152 x <64 x float>], [1152 x <64 x float>]* %5, i64 0, i64 0, i64 %731
  %733 = bitcast float* %732 to <64 x float>*
  %734 = load <64 x float>, <64 x float>* %733, align 16, !tbaa !439
  %735 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %710, <64 x float> %734, <64 x float> %722)
  %736 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %716, <64 x float> %734, <64 x float> %723)
  %737 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %729, <64 x float> %734, <64 x float> %724)
  %738 = add nuw nsw i64 %686, 5
  %739 = getelementptr inbounds [831744 x float], [831744 x float]* %6, i64 0, i64 %738
  %740 = load float, float* %739, align 4, !tbaa !433
  %741 = insertelement <64 x float> undef, float %740, i32 0
  %742 = shufflevector <64 x float> %741, <64 x float> undef, <64 x i32> zeroinitializer
  %743 = tail call <64 x float> @llvm.fmuladd.v64f32(<64 x float> %742, <64 x float> %734, <64 x float> %730)
  %indvars.iv.next.2 = add nuw nsw i64 %indvars.iv.2, 1
  %744 = add nuw nsw i32 %683, 1
  %exitcond.2 = icmp eq i64 %indvars.iv.next.2, 32
  br i1 %exitcond.2, label %for_end33.2, label %for_body32.2, !prof !34

for_end33.2:                                      ; preds = %for_body32.2
  %indvars.iv.next38 = add nuw nsw i64 %indvars.iv37, 1
  %exitcond39 = icmp eq i64 %indvars.iv.next38, 2
  br i1 %exitcond39, label %for_begin34.preheader, label %for_begin28.preheader, !prof !34
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
  %wide.load = load <4 x float>, <4 x float>* %7, align 4, !tbaa !456
  %8 = getelementptr inbounds float, float* %6, i64 4
  %9 = bitcast float* %8 to <4 x float>*
  %wide.load2 = load <4 x float>, <4 x float>* %9, align 4, !tbaa !456
  %10 = getelementptr inbounds float, float* %4, i64 %index
  %11 = bitcast float* %10 to <4 x float>*
  %wide.load3 = load <4 x float>, <4 x float>* %11, align 4, !tbaa !459
  %12 = getelementptr inbounds float, float* %10, i64 4
  %13 = bitcast float* %12 to <4 x float>*
  %wide.load4 = load <4 x float>, <4 x float>* %13, align 4, !tbaa !459
  %14 = fadd <4 x float> %wide.load, %wide.load3
  %15 = fadd <4 x float> %wide.load2, %wide.load4
  %16 = getelementptr inbounds float, float* %5, i64 %index
  %17 = bitcast float* %16 to <4 x float>*
  store <4 x float> %14, <4 x float>* %17, align 4, !tbaa !462
  %18 = getelementptr inbounds float, float* %16, i64 4
  %19 = bitcast float* %18 to <4 x float>*
  store <4 x float> %15, <4 x float>* %19, align 4, !tbaa !462
  %index.next = add i64 %index, 8
  %20 = icmp eq i64 %index.next, 1000
  br i1 %20, label %for_end, label %vector.body, !llvm.loop !465

for_end:                                          ; preds = %vector.body
  ret void
}

; Function Attrs: nounwind
define dllexport i32 @fused_nn_conv2d_8(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #2 {
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
  tail call fastcc void @fused_nn_conv2d_8_compute_(i8* %12, i8* %14, i8* %16)
  ret i32 0
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_conv2d_8_compute_(i8* noalias nocapture readonly, i8* noalias readonly, i8* noalias) unnamed_addr #3 {
entry:
  %3 = alloca [8 x <32 x float>], align 16
  %4 = alloca [224 x <32 x float>], align 16
  %5 = alloca [54 x <32 x float>], align 128
  %6 = alloca [153228 x float], align 16
  %7 = bitcast i8* %0 to float*
  br label %for_begin1.preheader

for_begin1.preheader:                             ; preds = %for_end3, %entry
  %indvar88 = phi i64 [ 0, %entry ], [ %indvar.next89, %for_end3 ]
  %8 = mul nuw nsw i64 %indvar88, 678
  %9 = trunc i64 %indvar88 to i32
  %10 = add i32 %9, -1
  %11 = icmp ult i32 %10, 224
  %12 = mul nuw nsw i64 %indvar88, 224
  %13 = add nsw i64 %12, -225
  br i1 %11, label %for_body5.us.us, label %for_begin4.preheader.preheader

for_begin4.preheader.preheader:                   ; preds = %for_begin1.preheader
  %scevgep95 = getelementptr [153228 x float], [153228 x float]* %6, i64 0, i64 %8
  %scevgep9596 = bitcast float* %scevgep95 to i8*
  call void @llvm.memset.p0i8.i64(i8* align 8 %scevgep9596, i8 0, i64 2712, i1 false)
  br label %for_end3

for_body5.us.us:                                  ; preds = %for_begin1.preheader, %if_end.us.us
  %indvars.iv97 = phi i64 [ %indvars.iv.next98, %if_end.us.us ], [ 0, %for_begin1.preheader ]
  %14 = phi i32 [ %21, %if_end.us.us ], [ 0, %for_begin1.preheader ]
  %15 = add nuw nsw i64 %8, %indvars.iv97
  %trunc.us.us = trunc i32 %14 to i31
  switch i31 %trunc.us.us, label %if_then.us.us [
    i31 225, label %if_end.us.us
    i31 0, label %if_end.us.us
  ]

if_then.us.us:                                    ; preds = %for_body5.us.us
  %16 = add nsw i64 %13, %indvars.iv97
  %17 = getelementptr inbounds float, float* %7, i64 %16
  %18 = load float, float* %17, align 4, !tbaa !466
  br label %if_end.us.us

if_end.us.us:                                     ; preds = %if_then.us.us, %for_body5.us.us, %for_body5.us.us
  %19 = phi float [ %18, %if_then.us.us ], [ 0.000000e+00, %for_body5.us.us ], [ 0.000000e+00, %for_body5.us.us ]
  %20 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %15
  store float %19, float* %20, align 4, !tbaa !469
  %indvars.iv.next98 = add nuw nsw i64 %indvars.iv97, 1
  %21 = add nuw nsw i32 %14, 1
  %exitcond99 = icmp eq i64 %indvars.iv.next98, 226
  br i1 %exitcond99, label %for_end6.us-lcssa.us.us, label %for_body5.us.us, !prof !34

for_end6.us-lcssa.us.us:                          ; preds = %if_end.us.us
  %22 = add nuw nsw i64 %8, 226
  %23 = add nuw nsw i64 %12, 49951
  br label %for_body5.us.us.1

for_begin7.preheader:                             ; preds = %for_end3
  %24 = bitcast [8 x <32 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0
  %25 = bitcast i8* %1 to float*
  br label %for_begin10.preheader

for_end3:                                         ; preds = %if_end.us.us.2, %for_begin4.preheader.preheader
  %indvar.next89 = add nuw nsw i64 %indvar88, 1
  %exitcond104 = icmp eq i64 %indvar.next89, 226
  br i1 %exitcond104, label %for_begin7.preheader, label %for_begin1.preheader, !prof !34

for_begin10.preheader:                            ; preds = %for_end12, %for_begin7.preheader
  %indvars.iv82 = phi i64 [ 0, %for_begin7.preheader ], [ %indvars.iv.next83, %for_end12 ]
  %26 = mul nuw nsw i64 %indvars.iv82, 288
  %27 = trunc i64 %indvars.iv82 to i32
  %28 = urem i32 %27, 3
  %29 = mul nuw nsw i32 %28, 3
  %30 = udiv i32 %27, 3
  %31 = mul nsw i32 %30, 864
  %32 = or i32 %29, %31
  %33 = zext i32 %32 to i64
  br label %for_begin13.preheader

for_begin16.preheader:                            ; preds = %for_end12
  %34 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 32
  %35 = bitcast float* %34 to <32 x float>*
  %36 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 64
  %37 = bitcast float* %36 to <32 x float>*
  %38 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 96
  %39 = bitcast float* %38 to <32 x float>*
  %40 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 128
  %41 = bitcast float* %40 to <32 x float>*
  %42 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 160
  %43 = bitcast float* %42 to <32 x float>*
  %44 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 192
  %45 = bitcast float* %44 to <32 x float>*
  %46 = getelementptr inbounds [8 x <32 x float>], [8 x <32 x float>]* %3, i64 0, i64 0, i64 224
  %47 = bitcast float* %46 to <32 x float>*
  %48 = bitcast i8* %2 to float*
  %49 = bitcast [8 x <32 x float>]* %3 to i8*
  br label %for_begin19.preheader

for_begin13.preheader:                            ; preds = %for_begin13.preheader, %for_begin10.preheader
  %indvars.iv79 = phi i64 [ 0, %for_begin10.preheader ], [ %indvars.iv.next80, %for_begin13.preheader ]
  %50 = mul nuw nsw i64 %indvars.iv79, 96
  %51 = add nuw nsw i64 %50, %26
  %52 = add nuw nsw i64 %indvars.iv79, %33
  %53 = add nuw nsw i64 %52, 27
  %54 = add nuw nsw i64 %52, 54
  %55 = add nuw nsw i64 %52, 81
  %56 = add nuw nsw i64 %52, 108
  %57 = add nuw nsw i64 %52, 135
  %58 = add nuw nsw i64 %52, 162
  %59 = add nuw nsw i64 %52, 189
  %60 = add nuw nsw i64 %52, 216
  %61 = add nuw nsw i64 %52, 243
  %62 = add nuw nsw i64 %52, 270
  %63 = add nuw nsw i64 %52, 297
  %64 = add nuw nsw i64 %52, 324
  %65 = add nuw nsw i64 %52, 351
  %66 = add nuw nsw i64 %52, 378
  %67 = add nuw nsw i64 %52, 405
  %68 = add nuw nsw i64 %52, 432
  %69 = add nuw nsw i64 %52, 459
  %70 = add nuw nsw i64 %52, 486
  %71 = add nuw nsw i64 %52, 513
  %72 = add nuw nsw i64 %52, 540
  %73 = add nuw nsw i64 %52, 567
  %74 = add nuw nsw i64 %52, 594
  %75 = add nuw nsw i64 %52, 621
  %76 = add nuw nsw i64 %52, 648
  %77 = add nuw nsw i64 %52, 675
  %78 = add nuw nsw i64 %52, 702
  %79 = add nuw nsw i64 %52, 729
  %80 = add nuw nsw i64 %52, 756
  %81 = add nuw nsw i64 %52, 783
  %82 = add nuw nsw i64 %52, 810
  %83 = add nuw nsw i64 %52, 837
  %84 = getelementptr inbounds float, float* %25, i64 %52
  %85 = load float, float* %84, align 4, !tbaa !472
  %86 = insertelement <32 x float> undef, float %85, i32 0
  %87 = getelementptr inbounds float, float* %25, i64 %53
  %88 = load float, float* %87, align 4, !tbaa !472
  %89 = insertelement <32 x float> %86, float %88, i32 1
  %90 = getelementptr inbounds float, float* %25, i64 %54
  %91 = load float, float* %90, align 4, !tbaa !472
  %92 = insertelement <32 x float> %89, float %91, i32 2
  %93 = getelementptr inbounds float, float* %25, i64 %55
  %94 = load float, float* %93, align 4, !tbaa !472
  %95 = insertelement <32 x float> %92, float %94, i32 3
  %96 = getelementptr inbounds float, float* %25, i64 %56
  %97 = load float, float* %96, align 4, !tbaa !472
  %98 = insertelement <32 x float> %95, float %97, i32 4
  %99 = getelementptr inbounds float, float* %25, i64 %57
  %100 = load float, float* %99, align 4, !tbaa !472
  %101 = insertelement <32 x float> %98, float %100, i32 5
  %102 = getelementptr inbounds float, float* %25, i64 %58
  %103 = load float, float* %102, align 4, !tbaa !472
  %104 = insertelement <32 x float> %101, float %103, i32 6
  %105 = getelementptr inbounds float, float* %25, i64 %59
  %106 = load float, float* %105, align 4, !tbaa !472
  %107 = insertelement <32 x float> %104, float %106, i32 7
  %108 = getelementptr inbounds float, float* %25, i64 %60
  %109 = load float, float* %108, align 4, !tbaa !472
  %110 = insertelement <32 x float> %107, float %109, i32 8
  %111 = getelementptr inbounds float, float* %25, i64 %61
  %112 = load float, float* %111, align 4, !tbaa !472
  %113 = insertelement <32 x float> %110, float %112, i32 9
  %114 = getelementptr inbounds float, float* %25, i64 %62
  %115 = load float, float* %114, align 4, !tbaa !472
  %116 = insertelement <32 x float> %113, float %115, i32 10
  %117 = getelementptr inbounds float, float* %25, i64 %63
  %118 = load float, float* %117, align 4, !tbaa !472
  %119 = insertelement <32 x float> %116, float %118, i32 11
  %120 = getelementptr inbounds float, float* %25, i64 %64
  %121 = load float, float* %120, align 4, !tbaa !472
  %122 = insertelement <32 x float> %119, float %121, i32 12
  %123 = getelementptr inbounds float, float* %25, i64 %65
  %124 = load float, float* %123, align 4, !tbaa !472
  %125 = insertelement <32 x float> %122, float %124, i32 13
  %126 = getelementptr inbounds float, float* %25, i64 %66
  %127 = load float, float* %126, align 4, !tbaa !472
  %128 = insertelement <32 x float> %125, float %127, i32 14
  %129 = getelementptr inbounds float, float* %25, i64 %67
  %130 = load float, float* %129, align 4, !tbaa !472
  %131 = insertelement <32 x float> %128, float %130, i32 15
  %132 = getelementptr inbounds float, float* %25, i64 %68
  %133 = load float, float* %132, align 4, !tbaa !472
  %134 = insertelement <32 x float> %131, float %133, i32 16
  %135 = getelementptr inbounds float, float* %25, i64 %69
  %136 = load float, float* %135, align 4, !tbaa !472
  %137 = insertelement <32 x float> %134, float %136, i32 17
  %138 = getelementptr inbounds float, float* %25, i64 %70
  %139 = load float, float* %138, align 4, !tbaa !472
  %140 = insertelement <32 x float> %137, float %139, i32 18
  %141 = getelementptr inbounds float, float* %25, i64 %71
  %142 = load float, float* %141, align 4, !tbaa !472
  %143 = insertelement <32 x float> %140, float %142, i32 19
  %144 = getelementptr inbounds float, float* %25, i64 %72
  %145 = load float, float* %144, align 4, !tbaa !472
  %146 = insertelement <32 x float> %143, float %145, i32 20
  %147 = getelementptr inbounds float, float* %25, i64 %73
  %148 = load float, float* %147, align 4, !tbaa !472
  %149 = insertelement <32 x float> %146, float %148, i32 21
  %150 = getelementptr inbounds float, float* %25, i64 %74
  %151 = load float, float* %150, align 4, !tbaa !472
  %152 = insertelement <32 x float> %149, float %151, i32 22
  %153 = getelementptr inbounds float, float* %25, i64 %75
  %154 = load float, float* %153, align 4, !tbaa !472
  %155 = insertelement <32 x float> %152, float %154, i32 23
  %156 = getelementptr inbounds float, float* %25, i64 %76
  %157 = load float, float* %156, align 4, !tbaa !472
  %158 = insertelement <32 x float> %155, float %157, i32 24
  %159 = getelementptr inbounds float, float* %25, i64 %77
  %160 = load float, float* %159, align 4, !tbaa !472
  %161 = insertelement <32 x float> %158, float %160, i32 25
  %162 = getelementptr inbounds float, float* %25, i64 %78
  %163 = load float, float* %162, align 4, !tbaa !472
  %164 = insertelement <32 x float> %161, float %163, i32 26
  %165 = getelementptr inbounds float, float* %25, i64 %79
  %166 = load float, float* %165, align 4, !tbaa !472
  %167 = insertelement <32 x float> %164, float %166, i32 27
  %168 = getelementptr inbounds float, float* %25, i64 %80
  %169 = load float, float* %168, align 4, !tbaa !472
  %170 = insertelement <32 x float> %167, float %169, i32 28
  %171 = getelementptr inbounds float, float* %25, i64 %81
  %172 = load float, float* %171, align 4, !tbaa !472
  %173 = insertelement <32 x float> %170, float %172, i32 29
  %174 = getelementptr inbounds float, float* %25, i64 %82
  %175 = load float, float* %174, align 4, !tbaa !472
  %176 = insertelement <32 x float> %173, float %175, i32 30
  %177 = getelementptr inbounds float, float* %25, i64 %83
  %178 = load float, float* %177, align 4, !tbaa !472
  %179 = insertelement <32 x float> %176, float %178, i32 31
  %180 = getelementptr inbounds [54 x <32 x float>], [54 x <32 x float>]* %5, i64 0, i64 0, i64 %51
  %181 = bitcast float* %180 to <32 x float>*
  store <32 x float> %179, <32 x float>* %181, align 128, !tbaa !475
  %182 = add nuw nsw i64 %51, 32
  %183 = add nuw nsw i64 %52, 9
  %184 = add nuw nsw i64 %52, 36
  %185 = add nuw nsw i64 %52, 63
  %186 = add nuw nsw i64 %52, 90
  %187 = add nuw nsw i64 %52, 117
  %188 = add nuw nsw i64 %52, 144
  %189 = add nuw nsw i64 %52, 171
  %190 = add nuw nsw i64 %52, 198
  %191 = add nuw nsw i64 %52, 225
  %192 = add nuw nsw i64 %52, 252
  %193 = add nuw nsw i64 %52, 279
  %194 = add nuw nsw i64 %52, 306
  %195 = add nuw nsw i64 %52, 333
  %196 = add nuw nsw i64 %52, 360
  %197 = add nuw nsw i64 %52, 387
  %198 = add nuw nsw i64 %52, 414
  %199 = add nuw nsw i64 %52, 441
  %200 = add nuw nsw i64 %52, 468
  %201 = add nuw nsw i64 %52, 495
  %202 = add nuw nsw i64 %52, 522
  %203 = add nuw nsw i64 %52, 549
  %204 = add nuw nsw i64 %52, 576
  %205 = add nuw nsw i64 %52, 603
  %206 = add nuw nsw i64 %52, 630
  %207 = add nuw nsw i64 %52, 657
  %208 = add nuw nsw i64 %52, 684
  %209 = add nuw nsw i64 %52, 711
  %210 = add nuw nsw i64 %52, 738
  %211 = add nuw nsw i64 %52, 765
  %212 = add nuw nsw i64 %52, 792
  %213 = add nuw nsw i64 %52, 819
  %214 = add nuw nsw i64 %52, 846
  %215 = getelementptr inbounds float, float* %25, i64 %183
  %216 = load float, float* %215, align 4, !tbaa !472
  %217 = insertelement <32 x float> undef, float %216, i32 0
  %218 = getelementptr inbounds float, float* %25, i64 %184
  %219 = load float, float* %218, align 4, !tbaa !472
  %220 = insertelement <32 x float> %217, float %219, i32 1
  %221 = getelementptr inbounds float, float* %25, i64 %185
  %222 = load float, float* %221, align 4, !tbaa !472
  %223 = insertelement <32 x float> %220, float %222, i32 2
  %224 = getelementptr inbounds float, float* %25, i64 %186
  %225 = load float, float* %224, align 4, !tbaa !472
  %226 = insertelement <32 x float> %223, float %225, i32 3
  %227 = getelementptr inbounds float, float* %25, i64 %187
  %228 = load float, float* %227, align 4, !tbaa !472
  %229 = insertelement <32 x float> %226, float %228, i32 4
  %230 = getelementptr inbounds float, float* %25, i64 %188
  %231 = load float, float* %230, align 4, !tbaa !472
  %232 = insertelement <32 x float> %229, float %231, i32 5
  %233 = getelementptr inbounds float, float* %25, i64 %189
  %234 = load float, float* %233, align 4, !tbaa !472
  %235 = insertelement <32 x float> %232, float %234, i32 6
  %236 = getelementptr inbounds float, float* %25, i64 %190
  %237 = load float, float* %236, align 4, !tbaa !472
  %238 = insertelement <32 x float> %235, float %237, i32 7
  %239 = getelementptr inbounds float, float* %25, i64 %191
  %240 = load float, float* %239, align 4, !tbaa !472
  %241 = insertelement <32 x float> %238, float %240, i32 8
  %242 = getelementptr inbounds float, float* %25, i64 %192
  %243 = load float, float* %242, align 4, !tbaa !472
  %244 = insertelement <32 x float> %241, float %243, i32 9
  %245 = getelementptr inbounds float, float* %25, i64 %193
  %246 = load float, float* %245, align 4, !tbaa !472
  %247 = insertelement <32 x float> %244, float %246, i32 10
  %248 = getelementptr inbounds float, float* %25, i64 %194
  %249 = load float, float* %248, align 4, !tbaa !472
  %250 = insertelement <32 x float> %247, float %249, i32 11
  %251 = getelementptr inbounds float, float* %25, i64 %195
  %252 = load float, float* %251, align 4, !tbaa !472
  %253 = insertelement <32 x float> %250, float %252, i32 12
  %254 = getelementptr inbounds float, float* %25, i64 %196
  %255 = load float, float* %254, align 4, !tbaa !472
  %256 = insertelement <32 x float> %253, float %255, i32 13
  %257 = getelementptr inbounds float, float* %25, i64 %197
  %258 = load float, float* %257, align 4, !tbaa !472
  %259 = insertelement <32 x float> %256, float %258, i32 14
  %260 = getelementptr inbounds float, float* %25, i64 %198
  %261 = load float, float* %260, align 4, !tbaa !472
  %262 = insertelement <32 x float> %259, float %261, i32 15
  %263 = getelementptr inbounds float, float* %25, i64 %199
  %264 = load float, float* %263, align 4, !tbaa !472
  %265 = insertelement <32 x float> %262, float %264, i32 16
  %266 = getelementptr inbounds float, float* %25, i64 %200
  %267 = load float, float* %266, align 4, !tbaa !472
  %268 = insertelement <32 x float> %265, float %267, i32 17
  %269 = getelementptr inbounds float, float* %25, i64 %201
  %270 = load float, float* %269, align 4, !tbaa !472
  %271 = insertelement <32 x float> %268, float %270, i32 18
  %272 = getelementptr inbounds float, float* %25, i64 %202
  %273 = load float, float* %272, align 4, !tbaa !472
  %274 = insertelement <32 x float> %271, float %273, i32 19
  %275 = getelementptr inbounds float, float* %25, i64 %203
  %276 = load float, float* %275, align 4, !tbaa !472
  %277 = insertelement <32 x float> %274, float %276, i32 20
  %278 = getelementptr inbounds float, float* %25, i64 %204
  %279 = load float, float* %278, align 4, !tbaa !472
  %280 = insertelement <32 x float> %277, float %279, i32 21
  %281 = getelementptr inbounds float, float* %25, i64 %205
  %282 = load float, float* %281, align 4, !tbaa !472
  %283 = insertelement <32 x float> %280, float %282, i32 22
  %284 = getelementptr inbounds float, float* %25, i64 %206
  %285 = load float, float* %284, align 4, !tbaa !472
  %286 = insertelement <32 x float> %283, float %285, i32 23
  %287 = getelementptr inbounds float, float* %25, i64 %207
  %288 = load float, float* %287, align 4, !tbaa !472
  %289 = insertelement <32 x float> %286, float %288, i32 24
  %290 = getelementptr inbounds float, float* %25, i64 %208
  %291 = load float, float* %290, align 4, !tbaa !472
  %292 = insertelement <32 x float> %289, float %291, i32 25
  %293 = getelementptr inbounds float, float* %25, i64 %209
  %294 = load float, float* %293, align 4, !tbaa !472
  %295 = insertelement <32 x float> %292, float %294, i32 26
  %296 = getelementptr inbounds float, float* %25, i64 %210
  %297 = load float, float* %296, align 4, !tbaa !472
  %298 = insertelement <32 x float> %295, float %297, i32 27
  %299 = getelementptr inbounds float, float* %25, i64 %211
  %300 = load float, float* %299, align 4, !tbaa !472
  %301 = insertelement <32 x float> %298, float %300, i32 28
  %302 = getelementptr inbounds float, float* %25, i64 %212
  %303 = load float, float* %302, align 4, !tbaa !472
  %304 = insertelement <32 x float> %301, float %303, i32 29
  %305 = getelementptr inbounds float, float* %25, i64 %213
  %306 = load float, float* %305, align 4, !tbaa !472
  %307 = insertelement <32 x float> %304, float %306, i32 30
  %308 = getelementptr inbounds float, float* %25, i64 %214
  %309 = load float, float* %308, align 4, !tbaa !472
  %310 = insertelement <32 x float> %307, float %309, i32 31
  %311 = getelementptr inbounds [54 x <32 x float>], [54 x <32 x float>]* %5, i64 0, i64 0, i64 %182
  %312 = bitcast float* %311 to <32 x float>*
  store <32 x float> %310, <32 x float>* %312, align 128, !tbaa !475
  %313 = add nuw nsw i64 %51, 64
  %314 = add nuw nsw i64 %52, 18
  %315 = add nuw nsw i64 %52, 45
  %316 = add nuw nsw i64 %52, 72
  %317 = add nuw nsw i64 %52, 99
  %318 = add nuw nsw i64 %52, 126
  %319 = add nuw nsw i64 %52, 153
  %320 = add nuw nsw i64 %52, 180
  %321 = add nuw nsw i64 %52, 207
  %322 = add nuw nsw i64 %52, 234
  %323 = add nuw nsw i64 %52, 261
  %324 = add nuw nsw i64 %52, 288
  %325 = add nuw nsw i64 %52, 315
  %326 = add nuw nsw i64 %52, 342
  %327 = add nuw nsw i64 %52, 369
  %328 = add nuw nsw i64 %52, 396
  %329 = add nuw nsw i64 %52, 423
  %330 = add nuw nsw i64 %52, 450
  %331 = add nuw nsw i64 %52, 477
  %332 = add nuw nsw i64 %52, 504
  %333 = add nuw nsw i64 %52, 531
  %334 = add nuw nsw i64 %52, 558
  %335 = add nuw nsw i64 %52, 585
  %336 = add nuw nsw i64 %52, 612
  %337 = add nuw nsw i64 %52, 639
  %338 = add nuw nsw i64 %52, 666
  %339 = add nuw nsw i64 %52, 693
  %340 = add nuw nsw i64 %52, 720
  %341 = add nuw nsw i64 %52, 747
  %342 = add nuw nsw i64 %52, 774
  %343 = add nuw nsw i64 %52, 801
  %344 = add nuw nsw i64 %52, 828
  %345 = add nuw nsw i64 %52, 855
  %346 = getelementptr inbounds float, float* %25, i64 %314
  %347 = load float, float* %346, align 4, !tbaa !472
  %348 = insertelement <32 x float> undef, float %347, i32 0
  %349 = getelementptr inbounds float, float* %25, i64 %315
  %350 = load float, float* %349, align 4, !tbaa !472
  %351 = insertelement <32 x float> %348, float %350, i32 1
  %352 = getelementptr inbounds float, float* %25, i64 %316
  %353 = load float, float* %352, align 4, !tbaa !472
  %354 = insertelement <32 x float> %351, float %353, i32 2
  %355 = getelementptr inbounds float, float* %25, i64 %317
  %356 = load float, float* %355, align 4, !tbaa !472
  %357 = insertelement <32 x float> %354, float %356, i32 3
  %358 = getelementptr inbounds float, float* %25, i64 %318
  %359 = load float, float* %358, align 4, !tbaa !472
  %360 = insertelement <32 x float> %357, float %359, i32 4
  %361 = getelementptr inbounds float, float* %25, i64 %319
  %362 = load float, float* %361, align 4, !tbaa !472
  %363 = insertelement <32 x float> %360, float %362, i32 5
  %364 = getelementptr inbounds float, float* %25, i64 %320
  %365 = load float, float* %364, align 4, !tbaa !472
  %366 = insertelement <32 x float> %363, float %365, i32 6
  %367 = getelementptr inbounds float, float* %25, i64 %321
  %368 = load float, float* %367, align 4, !tbaa !472
  %369 = insertelement <32 x float> %366, float %368, i32 7
  %370 = getelementptr inbounds float, float* %25, i64 %322
  %371 = load float, float* %370, align 4, !tbaa !472
  %372 = insertelement <32 x float> %369, float %371, i32 8
  %373 = getelementptr inbounds float, float* %25, i64 %323
  %374 = load float, float* %373, align 4, !tbaa !472
  %375 = insertelement <32 x float> %372, float %374, i32 9
  %376 = getelementptr inbounds float, float* %25, i64 %324
  %377 = load float, float* %376, align 4, !tbaa !472
  %378 = insertelement <32 x float> %375, float %377, i32 10
  %379 = getelementptr inbounds float, float* %25, i64 %325
  %380 = load float, float* %379, align 4, !tbaa !472
  %381 = insertelement <32 x float> %378, float %380, i32 11
  %382 = getelementptr inbounds float, float* %25, i64 %326
  %383 = load float, float* %382, align 4, !tbaa !472
  %384 = insertelement <32 x float> %381, float %383, i32 12
  %385 = getelementptr inbounds float, float* %25, i64 %327
  %386 = load float, float* %385, align 4, !tbaa !472
  %387 = insertelement <32 x float> %384, float %386, i32 13
  %388 = getelementptr inbounds float, float* %25, i64 %328
  %389 = load float, float* %388, align 4, !tbaa !472
  %390 = insertelement <32 x float> %387, float %389, i32 14
  %391 = getelementptr inbounds float, float* %25, i64 %329
  %392 = load float, float* %391, align 4, !tbaa !472
  %393 = insertelement <32 x float> %390, float %392, i32 15
  %394 = getelementptr inbounds float, float* %25, i64 %330
  %395 = load float, float* %394, align 4, !tbaa !472
  %396 = insertelement <32 x float> %393, float %395, i32 16
  %397 = getelementptr inbounds float, float* %25, i64 %331
  %398 = load float, float* %397, align 4, !tbaa !472
  %399 = insertelement <32 x float> %396, float %398, i32 17
  %400 = getelementptr inbounds float, float* %25, i64 %332
  %401 = load float, float* %400, align 4, !tbaa !472
  %402 = insertelement <32 x float> %399, float %401, i32 18
  %403 = getelementptr inbounds float, float* %25, i64 %333
  %404 = load float, float* %403, align 4, !tbaa !472
  %405 = insertelement <32 x float> %402, float %404, i32 19
  %406 = getelementptr inbounds float, float* %25, i64 %334
  %407 = load float, float* %406, align 4, !tbaa !472
  %408 = insertelement <32 x float> %405, float %407, i32 20
  %409 = getelementptr inbounds float, float* %25, i64 %335
  %410 = load float, float* %409, align 4, !tbaa !472
  %411 = insertelement <32 x float> %408, float %410, i32 21
  %412 = getelementptr inbounds float, float* %25, i64 %336
  %413 = load float, float* %412, align 4, !tbaa !472
  %414 = insertelement <32 x float> %411, float %413, i32 22
  %415 = getelementptr inbounds float, float* %25, i64 %337
  %416 = load float, float* %415, align 4, !tbaa !472
  %417 = insertelement <32 x float> %414, float %416, i32 23
  %418 = getelementptr inbounds float, float* %25, i64 %338
  %419 = load float, float* %418, align 4, !tbaa !472
  %420 = insertelement <32 x float> %417, float %419, i32 24
  %421 = getelementptr inbounds float, float* %25, i64 %339
  %422 = load float, float* %421, align 4, !tbaa !472
  %423 = insertelement <32 x float> %420, float %422, i32 25
  %424 = getelementptr inbounds float, float* %25, i64 %340
  %425 = load float, float* %424, align 4, !tbaa !472
  %426 = insertelement <32 x float> %423, float %425, i32 26
  %427 = getelementptr inbounds float, float* %25, i64 %341
  %428 = load float, float* %427, align 4, !tbaa !472
  %429 = insertelement <32 x float> %426, float %428, i32 27
  %430 = getelementptr inbounds float, float* %25, i64 %342
  %431 = load float, float* %430, align 4, !tbaa !472
  %432 = insertelement <32 x float> %429, float %431, i32 28
  %433 = getelementptr inbounds float, float* %25, i64 %343
  %434 = load float, float* %433, align 4, !tbaa !472
  %435 = insertelement <32 x float> %432, float %434, i32 29
  %436 = getelementptr inbounds float, float* %25, i64 %344
  %437 = load float, float* %436, align 4, !tbaa !472
  %438 = insertelement <32 x float> %435, float %437, i32 30
  %439 = getelementptr inbounds float, float* %25, i64 %345
  %440 = load float, float* %439, align 4, !tbaa !472
  %441 = insertelement <32 x float> %438, float %440, i32 31
  %442 = getelementptr inbounds [54 x <32 x float>], [54 x <32 x float>]* %5, i64 0, i64 0, i64 %313
  %443 = bitcast float* %442 to <32 x float>*
  store <32 x float> %441, <32 x float>* %443, align 128, !tbaa !475
  %indvars.iv.next80 = add nuw nsw i64 %indvars.iv79, 1
  %exitcond81 = icmp eq i64 %indvars.iv.next80, 3
  br i1 %exitcond81, label %for_end12, label %for_begin13.preheader, !prof !34

for_end12:                                        ; preds = %for_begin13.preheader
  %indvars.iv.next83 = add nuw nsw i64 %indvars.iv82, 1
  %exitcond84 = icmp eq i64 %indvars.iv.next83, 6
  br i1 %exitcond84, label %for_begin16.preheader, label %for_begin10.preheader, !prof !34

for_begin19.preheader:                            ; preds = %for_end36, %for_begin16.preheader
  %444 = phi i32 [ 0, %for_begin16.preheader ], [ %625, %for_end36 ]
  %445 = urem i32 %444, 224
  %446 = udiv i32 %444, 224
  %447 = mul nsw i32 %446, 864
  %448 = zext i32 %447 to i64
  br label %for_body20

for_end18:                                        ; preds = %for_end36
  ret void

for_begin34.preheader:                            ; preds = %for_begin31.preheader
  %449 = mul nuw nsw i32 %445, 224
  %450 = mul nsw i32 %446, 1605632
  %451 = add nuw nsw i32 %450, %449
  %452 = zext i32 %451 to i64
  br label %for_begin37.preheader

for_body20:                                       ; preds = %for_begin31.preheader, %for_begin19.preheader
  %indvar = phi i64 [ 0, %for_begin19.preheader ], [ %indvar.next, %for_begin31.preheader ]
  %453 = phi i32 [ 0, %for_begin19.preheader ], [ %456, %for_begin31.preheader ]
  %454 = shl i64 %indvar, 3
  %scevgep = getelementptr [224 x <32 x float>], [224 x <32 x float>]* %4, i64 0, i64 %454
  %scevgep67 = bitcast <32 x float>* %scevgep to i8*
  %455 = shl i32 %453, 3
  call void @llvm.memset.p0i8.i64(i8* nonnull align 16 %49, i8 0, i64 1024, i1 false)
  br label %for_begin25.preheader

for_begin31.preheader:                            ; preds = %for_end27
  store <32 x float> %578, <32 x float>* %.sub, align 16, !tbaa !478
  store <32 x float> %584, <32 x float>* %35, align 16, !tbaa !478
  store <32 x float> %590, <32 x float>* %37, align 16, !tbaa !478
  store <32 x float> %596, <32 x float>* %39, align 16, !tbaa !478
  store <32 x float> %602, <32 x float>* %41, align 16, !tbaa !478
  store <32 x float> %608, <32 x float>* %43, align 16, !tbaa !478
  store <32 x float> %614, <32 x float>* %45, align 16, !tbaa !478
  store <32 x float> %620, <32 x float>* %47, align 16, !tbaa !478
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* align 16 %scevgep67, i8* nonnull align 16 %24, i64 1024, i1 false)
  %456 = add nuw nsw i32 %453, 1
  %indvar.next = add nuw nsw i64 %indvar, 1
  %exitcond68 = icmp eq i64 %indvar.next, 28
  br i1 %exitcond68, label %for_begin34.preheader, label %for_body20, !prof !34

for_begin25.preheader:                            ; preds = %for_end27, %for_body20
  %indvars.iv61 = phi i64 [ 0, %for_body20 ], [ %indvars.iv.next62, %for_end27 ]
  %.lcssa16.lcssa45 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %620, %for_end27 ]
  %.lcssa14.lcssa43 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %614, %for_end27 ]
  %.lcssa12.lcssa41 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %608, %for_end27 ]
  %.lcssa10.lcssa39 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %602, %for_end27 ]
  %.lcssa8.lcssa37 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %596, %for_end27 ]
  %.lcssa6.lcssa36 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %590, %for_end27 ]
  %.lcssa4.lcssa34 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %584, %for_end27 ]
  %.lcssa.lcssa32 = phi <32 x float> [ zeroinitializer, %for_body20 ], [ %578, %for_end27 ]
  %457 = phi i32 [ 0, %for_body20 ], [ %621, %for_end27 ]
  %458 = add nuw nsw i32 %457, %445
  %459 = mul i32 %458, 678
  %460 = add nsw i32 %459, %455
  %461 = mul nuw nsw i64 %indvars.iv61, 288
  %462 = add nuw nsw i64 %461, %448
  %463 = sext i32 %460 to i64
  br label %for_begin28.preheader

for_begin28.preheader:                            ; preds = %for_begin28.preheader, %for_begin25.preheader
  %indvars.iv = phi i64 [ 0, %for_begin25.preheader ], [ %indvars.iv.next, %for_begin28.preheader ]
  %.lcssa1631 = phi <32 x float> [ %.lcssa16.lcssa45, %for_begin25.preheader ], [ %620, %for_begin28.preheader ]
  %.lcssa1429 = phi <32 x float> [ %.lcssa14.lcssa43, %for_begin25.preheader ], [ %614, %for_begin28.preheader ]
  %.lcssa1227 = phi <32 x float> [ %.lcssa12.lcssa41, %for_begin25.preheader ], [ %608, %for_begin28.preheader ]
  %.lcssa1025 = phi <32 x float> [ %.lcssa10.lcssa39, %for_begin25.preheader ], [ %602, %for_begin28.preheader ]
  %.lcssa823 = phi <32 x float> [ %.lcssa8.lcssa37, %for_begin25.preheader ], [ %596, %for_begin28.preheader ]
  %.lcssa621 = phi <32 x float> [ %.lcssa6.lcssa36, %for_begin25.preheader ], [ %590, %for_begin28.preheader ]
  %.lcssa420 = phi <32 x float> [ %.lcssa4.lcssa34, %for_begin25.preheader ], [ %584, %for_begin28.preheader ]
  %.lcssa18 = phi <32 x float> [ %.lcssa.lcssa32, %for_begin25.preheader ], [ %578, %for_begin28.preheader ]
  %464 = add nsw i64 %indvars.iv, %463
  %465 = mul nuw nsw i64 %indvars.iv, 96
  %466 = add nuw nsw i64 %462, %465
  %467 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %464
  %468 = load float, float* %467, align 4, !tbaa !469
  %469 = insertelement <32 x float> undef, float %468, i32 0
  %470 = shufflevector <32 x float> %469, <32 x float> undef, <32 x i32> zeroinitializer
  %471 = getelementptr inbounds [54 x <32 x float>], [54 x <32 x float>]* %5, i64 0, i64 0, i64 %466
  %472 = bitcast float* %471 to <32 x float>*
  %473 = load <32 x float>, <32 x float>* %472, align 128, !tbaa !475
  %474 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %470, <32 x float> %473, <32 x float> %.lcssa18)
  %475 = add nsw i64 %464, 1
  %476 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %475
  %477 = load float, float* %476, align 4, !tbaa !469
  %478 = insertelement <32 x float> undef, float %477, i32 0
  %479 = shufflevector <32 x float> %478, <32 x float> undef, <32 x i32> zeroinitializer
  %480 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %479, <32 x float> %473, <32 x float> %.lcssa420)
  %481 = add nsw i64 %464, 2
  %482 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %481
  %483 = load float, float* %482, align 4, !tbaa !469
  %484 = insertelement <32 x float> undef, float %483, i32 0
  %485 = shufflevector <32 x float> %484, <32 x float> undef, <32 x i32> zeroinitializer
  %486 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %485, <32 x float> %473, <32 x float> %.lcssa621)
  %487 = add nsw i64 %464, 3
  %488 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %487
  %489 = load float, float* %488, align 4, !tbaa !469
  %490 = insertelement <32 x float> undef, float %489, i32 0
  %491 = shufflevector <32 x float> %490, <32 x float> undef, <32 x i32> zeroinitializer
  %492 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %491, <32 x float> %473, <32 x float> %.lcssa823)
  %493 = add nsw i64 %464, 4
  %494 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %493
  %495 = load float, float* %494, align 4, !tbaa !469
  %496 = insertelement <32 x float> undef, float %495, i32 0
  %497 = shufflevector <32 x float> %496, <32 x float> undef, <32 x i32> zeroinitializer
  %498 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %497, <32 x float> %473, <32 x float> %.lcssa1025)
  %499 = add nsw i64 %464, 5
  %500 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %499
  %501 = load float, float* %500, align 4, !tbaa !469
  %502 = insertelement <32 x float> undef, float %501, i32 0
  %503 = shufflevector <32 x float> %502, <32 x float> undef, <32 x i32> zeroinitializer
  %504 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %503, <32 x float> %473, <32 x float> %.lcssa1227)
  %505 = add nsw i64 %464, 6
  %506 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %505
  %507 = load float, float* %506, align 4, !tbaa !469
  %508 = insertelement <32 x float> undef, float %507, i32 0
  %509 = shufflevector <32 x float> %508, <32 x float> undef, <32 x i32> zeroinitializer
  %510 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %509, <32 x float> %473, <32 x float> %.lcssa1429)
  %511 = add nsw i64 %464, 7
  %512 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %511
  %513 = load float, float* %512, align 4, !tbaa !469
  %514 = insertelement <32 x float> undef, float %513, i32 0
  %515 = shufflevector <32 x float> %514, <32 x float> undef, <32 x i32> zeroinitializer
  %516 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %515, <32 x float> %473, <32 x float> %.lcssa1631)
  %517 = add nsw i64 %464, 226
  %518 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %517
  %519 = load float, float* %518, align 4, !tbaa !469
  %520 = insertelement <32 x float> undef, float %519, i32 0
  %521 = shufflevector <32 x float> %520, <32 x float> undef, <32 x i32> zeroinitializer
  %522 = add nuw nsw i64 %466, 32
  %523 = getelementptr inbounds [54 x <32 x float>], [54 x <32 x float>]* %5, i64 0, i64 0, i64 %522
  %524 = bitcast float* %523 to <32 x float>*
  %525 = load <32 x float>, <32 x float>* %524, align 4, !tbaa !475
  %526 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %521, <32 x float> %525, <32 x float> %474)
  %527 = add nsw i64 %464, 227
  %528 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %527
  %529 = load float, float* %528, align 4, !tbaa !469
  %530 = insertelement <32 x float> undef, float %529, i32 0
  %531 = shufflevector <32 x float> %530, <32 x float> undef, <32 x i32> zeroinitializer
  %532 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %531, <32 x float> %525, <32 x float> %480)
  %533 = add nsw i64 %464, 228
  %534 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %533
  %535 = load float, float* %534, align 4, !tbaa !469
  %536 = insertelement <32 x float> undef, float %535, i32 0
  %537 = shufflevector <32 x float> %536, <32 x float> undef, <32 x i32> zeroinitializer
  %538 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %537, <32 x float> %525, <32 x float> %486)
  %539 = add nsw i64 %464, 229
  %540 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %539
  %541 = load float, float* %540, align 4, !tbaa !469
  %542 = insertelement <32 x float> undef, float %541, i32 0
  %543 = shufflevector <32 x float> %542, <32 x float> undef, <32 x i32> zeroinitializer
  %544 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %543, <32 x float> %525, <32 x float> %492)
  %545 = add nsw i64 %464, 230
  %546 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %545
  %547 = load float, float* %546, align 4, !tbaa !469
  %548 = insertelement <32 x float> undef, float %547, i32 0
  %549 = shufflevector <32 x float> %548, <32 x float> undef, <32 x i32> zeroinitializer
  %550 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %549, <32 x float> %525, <32 x float> %498)
  %551 = add nsw i64 %464, 231
  %552 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %551
  %553 = load float, float* %552, align 4, !tbaa !469
  %554 = insertelement <32 x float> undef, float %553, i32 0
  %555 = shufflevector <32 x float> %554, <32 x float> undef, <32 x i32> zeroinitializer
  %556 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %555, <32 x float> %525, <32 x float> %504)
  %557 = add nsw i64 %464, 232
  %558 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %557
  %559 = load float, float* %558, align 4, !tbaa !469
  %560 = insertelement <32 x float> undef, float %559, i32 0
  %561 = shufflevector <32 x float> %560, <32 x float> undef, <32 x i32> zeroinitializer
  %562 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %561, <32 x float> %525, <32 x float> %510)
  %563 = add nsw i64 %464, 233
  %564 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %563
  %565 = load float, float* %564, align 4, !tbaa !469
  %566 = insertelement <32 x float> undef, float %565, i32 0
  %567 = shufflevector <32 x float> %566, <32 x float> undef, <32 x i32> zeroinitializer
  %568 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %567, <32 x float> %525, <32 x float> %516)
  %569 = add nsw i64 %464, 452
  %570 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %569
  %571 = load float, float* %570, align 4, !tbaa !469
  %572 = insertelement <32 x float> undef, float %571, i32 0
  %573 = shufflevector <32 x float> %572, <32 x float> undef, <32 x i32> zeroinitializer
  %574 = add nuw nsw i64 %466, 64
  %575 = getelementptr inbounds [54 x <32 x float>], [54 x <32 x float>]* %5, i64 0, i64 0, i64 %574
  %576 = bitcast float* %575 to <32 x float>*
  %577 = load <32 x float>, <32 x float>* %576, align 4, !tbaa !475
  %578 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %573, <32 x float> %577, <32 x float> %526)
  %579 = add nsw i64 %464, 453
  %580 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %579
  %581 = load float, float* %580, align 4, !tbaa !469
  %582 = insertelement <32 x float> undef, float %581, i32 0
  %583 = shufflevector <32 x float> %582, <32 x float> undef, <32 x i32> zeroinitializer
  %584 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %583, <32 x float> %577, <32 x float> %532)
  %585 = add nsw i64 %464, 454
  %586 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %585
  %587 = load float, float* %586, align 4, !tbaa !469
  %588 = insertelement <32 x float> undef, float %587, i32 0
  %589 = shufflevector <32 x float> %588, <32 x float> undef, <32 x i32> zeroinitializer
  %590 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %589, <32 x float> %577, <32 x float> %538)
  %591 = add nsw i64 %464, 455
  %592 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %591
  %593 = load float, float* %592, align 4, !tbaa !469
  %594 = insertelement <32 x float> undef, float %593, i32 0
  %595 = shufflevector <32 x float> %594, <32 x float> undef, <32 x i32> zeroinitializer
  %596 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %595, <32 x float> %577, <32 x float> %544)
  %597 = add nsw i64 %464, 456
  %598 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %597
  %599 = load float, float* %598, align 4, !tbaa !469
  %600 = insertelement <32 x float> undef, float %599, i32 0
  %601 = shufflevector <32 x float> %600, <32 x float> undef, <32 x i32> zeroinitializer
  %602 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %601, <32 x float> %577, <32 x float> %550)
  %603 = add nsw i64 %464, 457
  %604 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %603
  %605 = load float, float* %604, align 4, !tbaa !469
  %606 = insertelement <32 x float> undef, float %605, i32 0
  %607 = shufflevector <32 x float> %606, <32 x float> undef, <32 x i32> zeroinitializer
  %608 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %607, <32 x float> %577, <32 x float> %556)
  %609 = add nsw i64 %464, 458
  %610 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %609
  %611 = load float, float* %610, align 4, !tbaa !469
  %612 = insertelement <32 x float> undef, float %611, i32 0
  %613 = shufflevector <32 x float> %612, <32 x float> undef, <32 x i32> zeroinitializer
  %614 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %613, <32 x float> %577, <32 x float> %562)
  %615 = add nsw i64 %464, 459
  %616 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %615
  %617 = load float, float* %616, align 4, !tbaa !469
  %618 = insertelement <32 x float> undef, float %617, i32 0
  %619 = shufflevector <32 x float> %618, <32 x float> undef, <32 x i32> zeroinitializer
  %620 = tail call <32 x float> @llvm.fmuladd.v32f32(<32 x float> %619, <32 x float> %577, <32 x float> %568)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 3
  br i1 %exitcond, label %for_end27, label %for_begin28.preheader, !prof !34

for_end27:                                        ; preds = %for_begin28.preheader
  %indvars.iv.next62 = add nuw nsw i64 %indvars.iv61, 1
  %621 = add nuw nsw i32 %457, 1
  %exitcond63 = icmp eq i64 %indvars.iv.next62, 3
  br i1 %exitcond63, label %for_begin31.preheader, label %for_begin25.preheader, !prof !34

for_begin37.preheader:                            ; preds = %for_end39, %for_begin34.preheader
  %indvars.iv72 = phi i64 [ 0, %for_begin34.preheader ], [ %indvars.iv.next73, %for_end39 ]
  %622 = shl i64 %indvars.iv72, 3
  %623 = add nuw nsw i64 %622, %452
  %624 = shl i64 %indvars.iv72, 8
  br label %for_body38

for_end36:                                        ; preds = %for_end39
  %625 = add nuw nsw i32 %444, 1
  %exitcond75 = icmp eq i32 %625, 448
  br i1 %exitcond75, label %for_end18, label %for_begin19.preheader, !prof !34

for_body38:                                       ; preds = %for_body38, %for_begin37.preheader
  %indvars.iv69 = phi i64 [ 0, %for_begin37.preheader ], [ %indvars.iv.next70, %for_body38 ]
  %626 = add nuw nsw i64 %623, %indvars.iv69
  %627 = add nuw nsw i64 %626, 50176
  %628 = add nuw nsw i64 %626, 100352
  %629 = add nuw nsw i64 %626, 150528
  %630 = add nuw nsw i64 %626, 200704
  %631 = add nuw nsw i64 %626, 250880
  %632 = add nuw nsw i64 %626, 301056
  %633 = add nuw nsw i64 %626, 351232
  %634 = add nuw nsw i64 %626, 401408
  %635 = add nuw nsw i64 %626, 451584
  %636 = add nuw nsw i64 %626, 501760
  %637 = add nuw nsw i64 %626, 551936
  %638 = add nuw nsw i64 %626, 602112
  %639 = add nuw nsw i64 %626, 652288
  %640 = add nuw nsw i64 %626, 702464
  %641 = add nuw nsw i64 %626, 752640
  %642 = add nuw nsw i64 %626, 802816
  %643 = add nuw nsw i64 %626, 852992
  %644 = add nuw nsw i64 %626, 903168
  %645 = add nuw nsw i64 %626, 953344
  %646 = add nuw nsw i64 %626, 1003520
  %647 = add nuw nsw i64 %626, 1053696
  %648 = add nuw nsw i64 %626, 1103872
  %649 = add nuw nsw i64 %626, 1154048
  %650 = add nuw nsw i64 %626, 1204224
  %651 = add nuw nsw i64 %626, 1254400
  %652 = add nuw nsw i64 %626, 1304576
  %653 = add nuw nsw i64 %626, 1354752
  %654 = add nuw nsw i64 %626, 1404928
  %655 = add nuw nsw i64 %626, 1455104
  %656 = add nuw nsw i64 %626, 1505280
  %657 = add nuw nsw i64 %626, 1555456
  %658 = shl i64 %indvars.iv69, 5
  %659 = add nuw nsw i64 %658, %624
  %660 = getelementptr inbounds [224 x <32 x float>], [224 x <32 x float>]* %4, i64 0, i64 0, i64 %659
  %661 = bitcast float* %660 to <32 x float>*
  %662 = load <32 x float>, <32 x float>* %661, align 16, !tbaa !487
  %663 = getelementptr inbounds float, float* %48, i64 %626
  %664 = extractelement <32 x float> %662, i64 0
  store float %664, float* %663, align 4, !tbaa !490
  %665 = getelementptr inbounds float, float* %48, i64 %627
  %666 = extractelement <32 x float> %662, i64 1
  store float %666, float* %665, align 4, !tbaa !490
  %667 = getelementptr inbounds float, float* %48, i64 %628
  %668 = extractelement <32 x float> %662, i64 2
  store float %668, float* %667, align 4, !tbaa !490
  %669 = getelementptr inbounds float, float* %48, i64 %629
  %670 = extractelement <32 x float> %662, i64 3
  store float %670, float* %669, align 4, !tbaa !490
  %671 = getelementptr inbounds float, float* %48, i64 %630
  %672 = extractelement <32 x float> %662, i64 4
  store float %672, float* %671, align 4, !tbaa !490
  %673 = getelementptr inbounds float, float* %48, i64 %631
  %674 = extractelement <32 x float> %662, i64 5
  store float %674, float* %673, align 4, !tbaa !490
  %675 = getelementptr inbounds float, float* %48, i64 %632
  %676 = extractelement <32 x float> %662, i64 6
  store float %676, float* %675, align 4, !tbaa !490
  %677 = getelementptr inbounds float, float* %48, i64 %633
  %678 = extractelement <32 x float> %662, i64 7
  store float %678, float* %677, align 4, !tbaa !490
  %679 = getelementptr inbounds float, float* %48, i64 %634
  %680 = extractelement <32 x float> %662, i64 8
  store float %680, float* %679, align 4, !tbaa !490
  %681 = getelementptr inbounds float, float* %48, i64 %635
  %682 = extractelement <32 x float> %662, i64 9
  store float %682, float* %681, align 4, !tbaa !490
  %683 = getelementptr inbounds float, float* %48, i64 %636
  %684 = extractelement <32 x float> %662, i64 10
  store float %684, float* %683, align 4, !tbaa !490
  %685 = getelementptr inbounds float, float* %48, i64 %637
  %686 = extractelement <32 x float> %662, i64 11
  store float %686, float* %685, align 4, !tbaa !490
  %687 = getelementptr inbounds float, float* %48, i64 %638
  %688 = extractelement <32 x float> %662, i64 12
  store float %688, float* %687, align 4, !tbaa !490
  %689 = getelementptr inbounds float, float* %48, i64 %639
  %690 = extractelement <32 x float> %662, i64 13
  store float %690, float* %689, align 4, !tbaa !490
  %691 = getelementptr inbounds float, float* %48, i64 %640
  %692 = extractelement <32 x float> %662, i64 14
  store float %692, float* %691, align 4, !tbaa !490
  %693 = getelementptr inbounds float, float* %48, i64 %641
  %694 = extractelement <32 x float> %662, i64 15
  store float %694, float* %693, align 4, !tbaa !490
  %695 = getelementptr inbounds float, float* %48, i64 %642
  %696 = extractelement <32 x float> %662, i64 16
  store float %696, float* %695, align 4, !tbaa !490
  %697 = getelementptr inbounds float, float* %48, i64 %643
  %698 = extractelement <32 x float> %662, i64 17
  store float %698, float* %697, align 4, !tbaa !490
  %699 = getelementptr inbounds float, float* %48, i64 %644
  %700 = extractelement <32 x float> %662, i64 18
  store float %700, float* %699, align 4, !tbaa !490
  %701 = getelementptr inbounds float, float* %48, i64 %645
  %702 = extractelement <32 x float> %662, i64 19
  store float %702, float* %701, align 4, !tbaa !490
  %703 = getelementptr inbounds float, float* %48, i64 %646
  %704 = extractelement <32 x float> %662, i64 20
  store float %704, float* %703, align 4, !tbaa !490
  %705 = getelementptr inbounds float, float* %48, i64 %647
  %706 = extractelement <32 x float> %662, i64 21
  store float %706, float* %705, align 4, !tbaa !490
  %707 = getelementptr inbounds float, float* %48, i64 %648
  %708 = extractelement <32 x float> %662, i64 22
  store float %708, float* %707, align 4, !tbaa !490
  %709 = getelementptr inbounds float, float* %48, i64 %649
  %710 = extractelement <32 x float> %662, i64 23
  store float %710, float* %709, align 4, !tbaa !490
  %711 = getelementptr inbounds float, float* %48, i64 %650
  %712 = extractelement <32 x float> %662, i64 24
  store float %712, float* %711, align 4, !tbaa !490
  %713 = getelementptr inbounds float, float* %48, i64 %651
  %714 = extractelement <32 x float> %662, i64 25
  store float %714, float* %713, align 4, !tbaa !490
  %715 = getelementptr inbounds float, float* %48, i64 %652
  %716 = extractelement <32 x float> %662, i64 26
  store float %716, float* %715, align 4, !tbaa !490
  %717 = getelementptr inbounds float, float* %48, i64 %653
  %718 = extractelement <32 x float> %662, i64 27
  store float %718, float* %717, align 4, !tbaa !490
  %719 = getelementptr inbounds float, float* %48, i64 %654
  %720 = extractelement <32 x float> %662, i64 28
  store float %720, float* %719, align 4, !tbaa !490
  %721 = getelementptr inbounds float, float* %48, i64 %655
  %722 = extractelement <32 x float> %662, i64 29
  store float %722, float* %721, align 4, !tbaa !490
  %723 = getelementptr inbounds float, float* %48, i64 %656
  %724 = extractelement <32 x float> %662, i64 30
  store float %724, float* %723, align 4, !tbaa !490
  %725 = getelementptr inbounds float, float* %48, i64 %657
  %726 = extractelement <32 x float> %662, i64 31
  store float %726, float* %725, align 4, !tbaa !490
  %indvars.iv.next70 = add nuw nsw i64 %indvars.iv69, 1
  %exitcond71 = icmp eq i64 %indvars.iv.next70, 8
  br i1 %exitcond71, label %for_end39, label %for_body38, !prof !34

for_end39:                                        ; preds = %for_body38
  %indvars.iv.next73 = add nuw nsw i64 %indvars.iv72, 1
  %exitcond74 = icmp eq i64 %indvars.iv.next73, 28
  br i1 %exitcond74, label %for_end36, label %for_begin37.preheader, !prof !34

for_body5.us.us.1:                                ; preds = %if_end.us.us.1, %for_end6.us-lcssa.us.us
  %indvars.iv97.1 = phi i64 [ 0, %for_end6.us-lcssa.us.us ], [ %indvars.iv.next98.1, %if_end.us.us.1 ]
  %727 = phi i32 [ 0, %for_end6.us-lcssa.us.us ], [ %734, %if_end.us.us.1 ]
  %728 = add nuw nsw i64 %22, %indvars.iv97.1
  %trunc.us.us.1 = trunc i32 %727 to i31
  switch i31 %trunc.us.us.1, label %if_then.us.us.1 [
    i31 225, label %if_end.us.us.1
    i31 0, label %if_end.us.us.1
  ]

if_then.us.us.1:                                  ; preds = %for_body5.us.us.1
  %729 = add nuw nsw i64 %23, %indvars.iv97.1
  %730 = getelementptr inbounds float, float* %7, i64 %729
  %731 = load float, float* %730, align 4, !tbaa !466
  br label %if_end.us.us.1

if_end.us.us.1:                                   ; preds = %if_then.us.us.1, %for_body5.us.us.1, %for_body5.us.us.1
  %732 = phi float [ %731, %if_then.us.us.1 ], [ 0.000000e+00, %for_body5.us.us.1 ], [ 0.000000e+00, %for_body5.us.us.1 ]
  %733 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %728
  store float %732, float* %733, align 4, !tbaa !469
  %indvars.iv.next98.1 = add nuw nsw i64 %indvars.iv97.1, 1
  %734 = add nuw nsw i32 %727, 1
  %exitcond99.1 = icmp eq i64 %indvars.iv.next98.1, 226
  br i1 %exitcond99.1, label %for_end6.us-lcssa.us.us.1, label %for_body5.us.us.1, !prof !34

for_end6.us-lcssa.us.us.1:                        ; preds = %if_end.us.us.1
  %735 = add nuw nsw i64 %8, 452
  %736 = add nuw nsw i64 %12, 100127
  br label %for_body5.us.us.2

for_body5.us.us.2:                                ; preds = %if_end.us.us.2, %for_end6.us-lcssa.us.us.1
  %indvars.iv97.2 = phi i64 [ 0, %for_end6.us-lcssa.us.us.1 ], [ %indvars.iv.next98.2, %if_end.us.us.2 ]
  %737 = phi i32 [ 0, %for_end6.us-lcssa.us.us.1 ], [ %744, %if_end.us.us.2 ]
  %738 = add nuw nsw i64 %735, %indvars.iv97.2
  %trunc.us.us.2 = trunc i32 %737 to i31
  switch i31 %trunc.us.us.2, label %if_then.us.us.2 [
    i31 225, label %if_end.us.us.2
    i31 0, label %if_end.us.us.2
  ]

if_then.us.us.2:                                  ; preds = %for_body5.us.us.2
  %739 = add nuw nsw i64 %736, %indvars.iv97.2
  %740 = getelementptr inbounds float, float* %7, i64 %739
  %741 = load float, float* %740, align 4, !tbaa !466
  br label %if_end.us.us.2

if_end.us.us.2:                                   ; preds = %if_then.us.us.2, %for_body5.us.us.2, %for_body5.us.us.2
  %742 = phi float [ %741, %if_then.us.us.2 ], [ 0.000000e+00, %for_body5.us.us.2 ], [ 0.000000e+00, %for_body5.us.us.2 ]
  %743 = getelementptr inbounds [153228 x float], [153228 x float]* %6, i64 0, i64 %738
  store float %742, float* %743, align 4, !tbaa !469
  %indvars.iv.next98.2 = add nuw nsw i64 %indvars.iv97.2, 1
  %744 = add nuw nsw i32 %737, 1
  %exitcond99.2 = icmp eq i64 %indvars.iv.next98.2, 226
  br i1 %exitcond99.2, label %for_end3, label %for_body5.us.us.2, !prof !34
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
!6 = !{!"0x58cb190.w1.b0", !7, i64 0}
!7 = !{!"0x58cb190.w2.b0", !8, i64 0}
!8 = !{!"0x58cb190.w4.b0", !9, i64 0}
!9 = !{!"0x58cb190.w8.b0", !10, i64 0}
!10 = !{!"0x58cb190.w16.b0", !11, i64 0}
!11 = !{!"0x58cb190.w32.b0", !12, i64 0}
!12 = !{!"0x58cb190.w64.b0", !13, i64 0}
!13 = !{!"0x58cb190.w128.b0", !14, i64 0}
!14 = !{!"0x58cb190.w256.b0", !15, i64 0}
!15 = !{!"0x58cb190.w512.b0", !16, i64 0}
!16 = !{!"0x58cb190.w1024.b0", !17, i64 0}
!17 = !{!"float32", !18, i64 0}
!18 = !{!"0x58cb190", !19, i64 0}
!19 = !{!"tvm-tbaa"}
!20 = !{!21, !21, i64 0}
!21 = !{!"float32", !22, i64 0}
!22 = !{!"0x58cb740", !19, i64 0}
!23 = !{!24, !24, i64 0}
!24 = !{!"float32", !25, i64 0}
!25 = !{!"0x58cb780", !19, i64 0}
!26 = distinct !{!26, !27}
!27 = !{!"llvm.loop.isvectorized", i32 1}
!28 = !{!29, !29, i64 0}
!29 = !{!"float32", !30, i64 0}
!30 = !{!"0x32980e0", !19, i64 0}
!31 = !{!32, !32, i64 0}
!32 = !{!"float32", !33, i64 0}
!33 = !{!"0x328fb60", !19, i64 0}
!34 = !{!"branch_weights", i32 1, i32 1048576}
!35 = !{!36, !36, i64 0}
!36 = !{!"float32", !37, i64 0}
!37 = !{!"0x3600e50", !19, i64 0}
!38 = !{!39, !39, i64 0}
!39 = !{!"float32", !40, i64 0}
!40 = !{!"0x35ffbf0", !19, i64 0}
!41 = !{!42, !42, i64 0}
!42 = !{!"float32", !43, i64 0}
!43 = !{!"0x3600dc0", !19, i64 0}
!44 = !{!45, !45, i64 0}
!45 = !{!"float32", !46, i64 0}
!46 = !{!"0x339c380", !19, i64 0}
!47 = !{!48, !48, i64 0}
!48 = !{!"float32", !49, i64 0}
!49 = !{!"0x339c340", !19, i64 0}
!50 = !{!51, !51, i64 0}
!51 = !{!"float32", !52, i64 0}
!52 = !{!"0x35f8440", !19, i64 0}
!53 = !{!54, !54, i64 0}
!54 = !{!"float32", !55, i64 0}
!55 = !{!"0x35f9580", !19, i64 0}
!56 = !{!57, !57, i64 0}
!57 = !{!"float32", !58, i64 0}
!58 = !{!"0x35c8470", !19, i64 0}
!59 = !{!60, !60, i64 0}
!60 = !{!"float32", !61, i64 0}
!61 = !{!"0x35c8a90", !19, i64 0}
!62 = !{!63, !63, i64 0}
!63 = !{!"float32", !64, i64 0}
!64 = !{!"0x35c7d20", !19, i64 0}
!65 = !{!66, !66, i64 0}
!66 = !{!"float32", !67, i64 0}
!67 = !{!"0x32ccc10", !19, i64 0}
!68 = !{!69, !69, i64 0}
!69 = !{!"float32", !70, i64 0}
!70 = !{!"0x32cb030", !19, i64 0}
!71 = !{!72, !72, i64 0}
!72 = !{!"float32", !73, i64 0}
!73 = !{!"0x32ca470", !19, i64 0}
!74 = !{!75, !75, i64 0}
!75 = !{!"float32", !76, i64 0}
!76 = !{!"0x32cc950", !19, i64 0}
!77 = !{!78, !78, i64 0}
!78 = !{!"0x35c6ce0.w64.b0", !79, i64 0}
!79 = !{!"0x35c6ce0.w128.b0", !80, i64 0}
!80 = !{!"0x35c6ce0.w256.b0", !81, i64 0}
!81 = !{!"0x35c6ce0.w512.b0", !82, i64 0}
!82 = !{!"0x35c6ce0.w1024.b0", !83, i64 0}
!83 = !{!"float32", !84, i64 0}
!84 = !{!"0x35c6ce0", !19, i64 0}
!85 = !{!86, !86, i64 0}
!86 = !{!"float32", !87, i64 0}
!87 = !{!"0x3212560", !19, i64 0}
!88 = !{!89, !89, i64 0}
!89 = !{!"float32", !90, i64 0}
!90 = !{!"0x3390500", !19, i64 0}
!91 = !{!92, !92, i64 0}
!92 = !{!"0x35ae2a0.w1.b0", !93, i64 0}
!93 = !{!"0x35ae2a0.w2.b0", !94, i64 0}
!94 = !{!"0x35ae2a0.w4.b0", !95, i64 0}
!95 = !{!"0x35ae2a0.w8.b0", !96, i64 0}
!96 = !{!"0x35ae2a0.w16.b0", !97, i64 0}
!97 = !{!"0x35ae2a0.w32.b0", !98, i64 0}
!98 = !{!"0x35ae2a0.w64.b0", !99, i64 0}
!99 = !{!"0x35ae2a0.w128.b0", !100, i64 0}
!100 = !{!"0x35ae2a0.w256.b0", !101, i64 0}
!101 = !{!"0x35ae2a0.w512.b0", !102, i64 0}
!102 = !{!"0x35ae2a0.w1024.b0", !103, i64 0}
!103 = !{!"float32", !104, i64 0}
!104 = !{!"0x35ae2a0", !19, i64 0}
!105 = !{!106, !106, i64 0}
!106 = !{!"float32", !107, i64 0}
!107 = !{!"0x3232e40", !19, i64 0}
!108 = !{!109, !109, i64 0}
!109 = !{!"float32", !110, i64 0}
!110 = !{!"0x35aad30", !19, i64 0}
!111 = distinct !{!111, !27}
!112 = !{!113, !113, i64 0}
!113 = !{!"float32", !114, i64 0}
!114 = !{!"0x33b4550", !19, i64 0}
!115 = !{!116, !116, i64 0}
!116 = !{!"float32", !117, i64 0}
!117 = !{!"0x33b5640", !19, i64 0}
!118 = !{!119, !119, i64 0}
!119 = !{!"float32", !120, i64 0}
!120 = !{!"0x33bcc90", !19, i64 0}
!121 = !{!122, !122, i64 0}
!122 = !{!"float32", !123, i64 0}
!123 = !{!"0x33bba30", !19, i64 0}
!124 = !{!125, !125, i64 0}
!125 = !{!"float32", !126, i64 0}
!126 = !{!"0x33bcc00", !19, i64 0}
!127 = !{!128, !128, i64 0}
!128 = !{!"float32", !129, i64 0}
!129 = !{!"0x32492a0", !19, i64 0}
!130 = !{!131, !131, i64 0}
!131 = !{!"float32", !132, i64 0}
!132 = !{!"0x35ab6c0", !19, i64 0}
!133 = !{!134, !134, i64 0}
!134 = !{!"float32", !135, i64 0}
!135 = !{!"0x35ab5a0", !19, i64 0}
!136 = distinct !{!136, !27}
!137 = !{!138, !138, i64 0}
!138 = !{!"float32", !139, i64 0}
!139 = !{!"0x35f2250", !19, i64 0}
!140 = !{!141, !141, i64 0}
!141 = !{!"float32", !142, i64 0}
!142 = !{!"0x35f2210", !19, i64 0}
!143 = !{!144, !144, i64 0}
!144 = !{!"float32", !145, i64 0}
!145 = !{!"0x325d580", !19, i64 0}
!146 = !{!147, !147, i64 0}
!147 = !{!"float32", !148, i64 0}
!148 = !{!"0x324b6a0", !19, i64 0}
!149 = !{!150, !150, i64 0}
!150 = !{!"float32", !151, i64 0}
!151 = !{!"0x325cd10", !19, i64 0}
!152 = !{!153, !153, i64 0}
!153 = !{!"0x35dc9e0.w1.b0", !154, i64 0}
!154 = !{!"0x35dc9e0.w2.b0", !155, i64 0}
!155 = !{!"0x35dc9e0.w4.b0", !156, i64 0}
!156 = !{!"0x35dc9e0.w8.b0", !157, i64 0}
!157 = !{!"0x35dc9e0.w16.b0", !158, i64 0}
!158 = !{!"0x35dc9e0.w32.b0", !159, i64 0}
!159 = !{!"0x35dc9e0.w64.b0", !160, i64 0}
!160 = !{!"0x35dc9e0.w128.b0", !161, i64 0}
!161 = !{!"0x35dc9e0.w256.b0", !162, i64 0}
!162 = !{!"0x35dc9e0.w512.b0", !163, i64 0}
!163 = !{!"0x35dc9e0.w1024.b0", !164, i64 0}
!164 = !{!"float32", !165, i64 0}
!165 = !{!"0x35dc9e0", !19, i64 0}
!166 = !{!167, !167, i64 0}
!167 = !{!"float32", !168, i64 0}
!168 = !{!"0x35dc9a0", !19, i64 0}
!169 = !{!170, !170, i64 0}
!170 = !{!"float32", !171, i64 0}
!171 = !{!"0x35dce20", !19, i64 0}
!172 = distinct !{!172, !27}
!173 = !{!174, !174, i64 0}
!174 = !{!"float32", !175, i64 0}
!175 = !{!"0x3299730", !19, i64 0}
!176 = !{!177, !177, i64 0}
!177 = !{!"float32", !178, i64 0}
!178 = !{!"0x3290860", !19, i64 0}
!179 = !{!180, !180, i64 0}
!180 = !{!"0x3298f40.w16.b0", !181, i64 0}
!181 = !{!"0x3298f40.w32.b0", !182, i64 0}
!182 = !{!"0x3298f40.w64.b0", !183, i64 0}
!183 = !{!"0x3298f40.w128.b0", !184, i64 0}
!184 = !{!"0x3298f40.w256.b0", !185, i64 0}
!185 = !{!"0x3298f40.w512.b0", !186, i64 0}
!186 = !{!"0x3298f40.w1024.b0", !187, i64 0}
!187 = !{!"float32", !188, i64 0}
!188 = !{!"0x3298f40", !19, i64 0}
!189 = !{!190, !190, i64 0}
!190 = !{!"float32", !191, i64 0}
!191 = !{!"0x3299ab0", !19, i64 0}
!192 = !{!187, !187, i64 0}
!193 = !{!194, !194, i64 0}
!194 = !{!"float32", !195, i64 0}
!195 = !{!"0x32993f0", !19, i64 0}
!196 = !{!197, !197, i64 0}
!197 = !{!"float32", !198, i64 0}
!198 = !{!"0x32921b0", !19, i64 0}
!199 = !{!200, !200, i64 0}
!200 = !{!"0x344d590.w1.b0", !201, i64 0}
!201 = !{!"0x344d590.w2.b0", !202, i64 0}
!202 = !{!"0x344d590.w4.b0", !203, i64 0}
!203 = !{!"0x344d590.w8.b0", !204, i64 0}
!204 = !{!"0x344d590.w16.b0", !205, i64 0}
!205 = !{!"0x344d590.w32.b0", !206, i64 0}
!206 = !{!"0x344d590.w64.b0", !207, i64 0}
!207 = !{!"0x344d590.w128.b0", !208, i64 0}
!208 = !{!"0x344d590.w256.b0", !209, i64 0}
!209 = !{!"0x344d590.w512.b0", !210, i64 0}
!210 = !{!"0x344d590.w1024.b0", !211, i64 0}
!211 = !{!"float32", !212, i64 0}
!212 = !{!"0x344d590", !19, i64 0}
!213 = !{!214, !214, i64 0}
!214 = !{!"float32", !215, i64 0}
!215 = !{!"0x344dc30", !19, i64 0}
!216 = !{!217, !217, i64 0}
!217 = !{!"float32", !218, i64 0}
!218 = !{!"0x344dc70", !19, i64 0}
!219 = distinct !{!219, !27}
!220 = !{!221, !221, i64 0}
!221 = !{!"float32", !222, i64 0}
!222 = !{!"0x329bac0", !19, i64 0}
!223 = !{!224, !224, i64 0}
!224 = !{!"float32", !225, i64 0}
!225 = !{!"0x329b790", !19, i64 0}
!226 = !{!227, !227, i64 0}
!227 = !{!"float32", !228, i64 0}
!228 = !{!"0x32d78a0", !19, i64 0}
!229 = !{!230, !230, i64 0}
!230 = !{!"float32", !231, i64 0}
!231 = !{!"0x32cf3b0", !19, i64 0}
!232 = !{!233, !233, i64 0}
!233 = !{!"0x32d7c50.w16.b0", !234, i64 0}
!234 = !{!"0x32d7c50.w32.b0", !235, i64 0}
!235 = !{!"0x32d7c50.w64.b0", !236, i64 0}
!236 = !{!"0x32d7c50.w128.b0", !237, i64 0}
!237 = !{!"0x32d7c50.w256.b0", !238, i64 0}
!238 = !{!"0x32d7c50.w512.b0", !239, i64 0}
!239 = !{!"0x32d7c50.w1024.b0", !240, i64 0}
!240 = !{!"float32", !241, i64 0}
!241 = !{!"0x32d7c50", !19, i64 0}
!242 = !{!243, !243, i64 0}
!243 = !{!"float32", !244, i64 0}
!244 = !{!"0x32d85c0", !19, i64 0}
!245 = !{!246, !246, i64 0}
!246 = !{!"float32", !247, i64 0}
!247 = !{!"0x32d8200", !19, i64 0}
!248 = !{!249, !249, i64 0}
!249 = !{!"float32", !250, i64 0}
!250 = !{!"0x32d8240", !19, i64 0}
!251 = !{!252, !252, i64 0}
!252 = !{!"float32", !253, i64 0}
!253 = !{!"0x32cef50", !19, i64 0}
!254 = !{!255, !255, i64 0}
!255 = !{!"float32", !256, i64 0}
!256 = !{!"0x3239970", !19, i64 0}
!257 = !{!258, !258, i64 0}
!258 = !{!"float32", !259, i64 0}
!259 = !{!"0x32100e0", !19, i64 0}
!260 = !{!261, !261, i64 0}
!261 = !{!"float32", !262, i64 0}
!262 = !{!"0x3211fa0", !19, i64 0}
!263 = !{!264, !264, i64 0}
!264 = !{!"float32", !265, i64 0}
!265 = !{!"0x33f6650", !19, i64 0}
!266 = !{!267, !267, i64 0}
!267 = !{!"float32", !268, i64 0}
!268 = !{!"0x33f53f0", !19, i64 0}
!269 = !{!270, !270, i64 0}
!270 = !{!"float32", !271, i64 0}
!271 = !{!"0x33f65c0", !19, i64 0}
!272 = !{!273, !273, i64 0}
!273 = !{!"float32", !274, i64 0}
!274 = !{!"0x32bf5b0", !19, i64 0}
!275 = !{!276, !276, i64 0}
!276 = !{!"float32", !277, i64 0}
!277 = !{!"0x32be350", !19, i64 0}
!278 = !{!279, !279, i64 0}
!279 = !{!"float32", !280, i64 0}
!280 = !{!"0x32bf520", !19, i64 0}
!281 = !{!282, !282, i64 0}
!282 = !{!"float32", !283, i64 0}
!283 = !{!"0x3374900", !19, i64 0}
!284 = !{!285, !285, i64 0}
!285 = !{!"float32", !286, i64 0}
!286 = !{!"0x337dc50", !19, i64 0}
!287 = !{!288, !288, i64 0}
!288 = !{!"float32", !289, i64 0}
!289 = !{!"0x337d560", !19, i64 0}
!290 = !{!291, !291, i64 0}
!291 = !{!"float32", !292, i64 0}
!292 = !{!"0x3374b50", !19, i64 0}
!293 = !{!294, !294, i64 0}
!294 = !{!"float32", !295, i64 0}
!295 = !{!"0x337d890", !19, i64 0}
!296 = !{!297, !297, i64 0}
!297 = !{!"float32", !298, i64 0}
!298 = !{!"0x337d8d0", !19, i64 0}
!299 = !{!300, !300, i64 0}
!300 = !{!"0x337d2e0.w32.b0", !301, i64 0}
!301 = !{!"0x337d2e0.w64.b0", !302, i64 0}
!302 = !{!"0x337d2e0.w128.b0", !303, i64 0}
!303 = !{!"0x337d2e0.w256.b0", !304, i64 0}
!304 = !{!"0x337d2e0.w512.b0", !305, i64 0}
!305 = !{!"0x337d2e0.w1024.b0", !306, i64 0}
!306 = !{!"float32", !307, i64 0}
!307 = !{!"0x337d2e0", !19, i64 0}
!308 = !{!309, !309, i64 0}
!309 = !{!"float32", !310, i64 0}
!310 = !{!"0x32db1b0", !19, i64 0}
!311 = !{!312, !312, i64 0}
!312 = !{!"float32", !313, i64 0}
!313 = !{!"0x32da500", !19, i64 0}
!314 = !{!315, !315, i64 0}
!315 = !{!"float32", !316, i64 0}
!316 = !{!"0x32d6ad0", !19, i64 0}
!317 = !{!318, !318, i64 0}
!318 = !{!"float32", !319, i64 0}
!319 = !{!"0x32db0d0", !19, i64 0}
!320 = !{!321, !321, i64 0}
!321 = !{!"float32", !322, i64 0}
!322 = !{!"0x32daa20", !19, i64 0}
!323 = !{!324, !324, i64 0}
!324 = !{!"0x32daac0.w32.b0", !325, i64 0}
!325 = !{!"0x32daac0.w64.b0", !326, i64 0}
!326 = !{!"0x32daac0.w128.b0", !327, i64 0}
!327 = !{!"0x32daac0.w256.b0", !328, i64 0}
!328 = !{!"0x32daac0.w512.b0", !329, i64 0}
!329 = !{!"0x32daac0.w1024.b0", !330, i64 0}
!330 = !{!"float32", !331, i64 0}
!331 = !{!"0x32daac0", !19, i64 0}
!332 = !{!333, !333, i64 0}
!333 = !{!"float32", !334, i64 0}
!334 = !{!"0x32dac30", !19, i64 0}
!335 = !{!336, !336, i64 0}
!336 = !{!"float32", !337, i64 0}
!337 = !{!"0x33edec0", !19, i64 0}
!338 = !{!339, !339, i64 0}
!339 = !{!"float32", !340, i64 0}
!340 = !{!"0x33ef000", !19, i64 0}
!341 = !{!342, !342, i64 0}
!342 = !{!"float32", !343, i64 0}
!343 = !{!"0x33e8e00", !19, i64 0}
!344 = !{!345, !345, i64 0}
!345 = !{!"float32", !346, i64 0}
!346 = !{!"0x33e8dc0", !19, i64 0}
!347 = distinct !{!347, !27}
!348 = !{!349, !349, i64 0}
!349 = !{!"float32", !350, i64 0}
!350 = !{!"0x33494a0", !19, i64 0}
!351 = !{!352, !352, i64 0}
!352 = !{!"float32", !353, i64 0}
!353 = !{!"0x32d9850", !19, i64 0}
!354 = !{!355, !355, i64 0}
!355 = !{!"float32", !356, i64 0}
!356 = !{!"0x335c9e0", !19, i64 0}
!357 = !{!358, !358, i64 0}
!358 = !{!"float32", !359, i64 0}
!359 = !{!"0x335db20", !19, i64 0}
!360 = !{!361, !361, i64 0}
!361 = !{!"float32", !362, i64 0}
!362 = !{!"0x33651b0", !19, i64 0}
!363 = !{!364, !364, i64 0}
!364 = !{!"float32", !365, i64 0}
!365 = !{!"0x3363f50", !19, i64 0}
!366 = !{!367, !367, i64 0}
!367 = !{!"float32", !368, i64 0}
!368 = !{!"0x3365120", !19, i64 0}
!369 = !{!370, !370, i64 0}
!370 = !{!"float32", !371, i64 0}
!371 = !{!"0x34087c0", !19, i64 0}
!372 = !{!373, !373, i64 0}
!373 = !{!"float32", !374, i64 0}
!374 = !{!"0x34058d0", !19, i64 0}
!375 = !{!376, !376, i64 0}
!376 = !{!"float32", !377, i64 0}
!377 = !{!"0x34055e0", !19, i64 0}
!378 = !{!379, !379, i64 0}
!379 = !{!"float32", !380, i64 0}
!380 = !{!"0x3408590", !19, i64 0}
!381 = !{!382, !382, i64 0}
!382 = !{!"0x340e660.w32.b0", !383, i64 0}
!383 = !{!"0x340e660.w64.b0", !384, i64 0}
!384 = !{!"0x340e660.w128.b0", !385, i64 0}
!385 = !{!"0x340e660.w256.b0", !386, i64 0}
!386 = !{!"0x340e660.w512.b0", !387, i64 0}
!387 = !{!"0x340e660.w1024.b0", !388, i64 0}
!388 = !{!"float32", !389, i64 0}
!389 = !{!"0x340e660", !19, i64 0}
!390 = !{!391, !391, i64 0}
!391 = !{!"float32", !392, i64 0}
!392 = !{!"0x340e360", !19, i64 0}
!393 = !{!394, !394, i64 0}
!394 = !{!"float32", !395, i64 0}
!395 = !{!"0x340ec00", !19, i64 0}
!396 = !{!397, !397, i64 0}
!397 = !{!"float32", !398, i64 0}
!398 = !{!"0x35b5280", !19, i64 0}
!399 = !{!400, !400, i64 0}
!400 = !{!"float32", !401, i64 0}
!401 = !{!"0x35b8d80", !19, i64 0}
!402 = distinct !{!402, !27}
!403 = !{!404, !404, i64 0}
!404 = !{!"float32", !405, i64 0}
!405 = !{!"0x3396830", !19, i64 0}
!406 = !{!407, !407, i64 0}
!407 = !{!"float32", !408, i64 0}
!408 = !{!"0x3393b60", !19, i64 0}
!409 = !{!410, !410, i64 0}
!410 = !{!"float32", !411, i64 0}
!411 = !{!"0x3393d90", !19, i64 0}
!412 = !{!413, !413, i64 0}
!413 = !{!"float32", !414, i64 0}
!414 = !{!"0x3396600", !19, i64 0}
!415 = !{!416, !416, i64 0}
!416 = !{!"0x339c450.w32.b0", !417, i64 0}
!417 = !{!"0x339c450.w64.b0", !418, i64 0}
!418 = !{!"0x339c450.w128.b0", !419, i64 0}
!419 = !{!"0x339c450.w256.b0", !420, i64 0}
!420 = !{!"0x339c450.w512.b0", !421, i64 0}
!421 = !{!"0x339c450.w1024.b0", !422, i64 0}
!422 = !{!"float32", !423, i64 0}
!423 = !{!"0x339c450", !19, i64 0}
!424 = !{!425, !425, i64 0}
!425 = !{!"float32", !426, i64 0}
!426 = !{!"0x339c150", !19, i64 0}
!427 = !{!428, !428, i64 0}
!428 = !{!"float32", !429, i64 0}
!429 = !{!"0x339c9f0", !19, i64 0}
!430 = !{!431, !431, i64 0}
!431 = !{!"float32", !432, i64 0}
!432 = !{!"0x33d72f0", !19, i64 0}
!433 = !{!434, !434, i64 0}
!434 = !{!"float32", !435, i64 0}
!435 = !{!"0x33d4620", !19, i64 0}
!436 = !{!437, !437, i64 0}
!437 = !{!"float32", !438, i64 0}
!438 = !{!"0x33d4330", !19, i64 0}
!439 = !{!440, !440, i64 0}
!440 = !{!"float32", !441, i64 0}
!441 = !{!"0x33d70c0", !19, i64 0}
!442 = !{!443, !443, i64 0}
!443 = !{!"0x33dccf0.w64.b0", !444, i64 0}
!444 = !{!"0x33dccf0.w128.b0", !445, i64 0}
!445 = !{!"0x33dccf0.w256.b0", !446, i64 0}
!446 = !{!"0x33dccf0.w512.b0", !447, i64 0}
!447 = !{!"0x33dccf0.w1024.b0", !448, i64 0}
!448 = !{!"float32", !449, i64 0}
!449 = !{!"0x33dccf0", !19, i64 0}
!450 = !{!451, !451, i64 0}
!451 = !{!"float32", !452, i64 0}
!452 = !{!"0x33dc9f0", !19, i64 0}
!453 = !{!454, !454, i64 0}
!454 = !{!"float32", !455, i64 0}
!455 = !{!"0x33dd290", !19, i64 0}
!456 = !{!457, !457, i64 0}
!457 = !{!"float32", !458, i64 0}
!458 = !{!"0x7b4c7c0", !19, i64 0}
!459 = !{!460, !460, i64 0}
!460 = !{!"float32", !461, i64 0}
!461 = !{!"0x3216b30", !19, i64 0}
!462 = !{!463, !463, i64 0}
!463 = !{!"float32", !464, i64 0}
!464 = !{!"0x3231520", !19, i64 0}
!465 = distinct !{!465, !27}
!466 = !{!467, !467, i64 0}
!467 = !{!"float32", !468, i64 0}
!468 = !{!"0x3426dd0", !19, i64 0}
!469 = !{!470, !470, i64 0}
!470 = !{!"float32", !471, i64 0}
!471 = !{!"0x342f940", !19, i64 0}
!472 = !{!473, !473, i64 0}
!473 = !{!"float32", !474, i64 0}
!474 = !{!"0x342f5c0", !19, i64 0}
!475 = !{!476, !476, i64 0}
!476 = !{!"float32", !477, i64 0}
!477 = !{!"0x3427020", !19, i64 0}
!478 = !{!479, !479, i64 0}
!479 = !{!"0x342efd0.w32.b0", !480, i64 0}
!480 = !{!"0x342efd0.w64.b0", !481, i64 0}
!481 = !{!"0x342efd0.w128.b0", !482, i64 0}
!482 = !{!"0x342efd0.w256.b0", !483, i64 0}
!483 = !{!"0x342efd0.w512.b0", !484, i64 0}
!484 = !{!"0x342efd0.w1024.b0", !485, i64 0}
!485 = !{!"float32", !486, i64 0}
!486 = !{!"0x342efd0", !19, i64 0}
!487 = !{!488, !488, i64 0}
!488 = !{!"float32", !489, i64 0}
!489 = !{!"0x342f580", !19, i64 0}
!490 = !{!491, !491, i64 0}
!491 = !{!"float32", !492, i64 0}
!492 = !{!"0x342f250", !19, i64 0}
