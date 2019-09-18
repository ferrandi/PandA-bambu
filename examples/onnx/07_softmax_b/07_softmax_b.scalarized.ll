; ModuleID = './07_softmax_b.ll'
source_filename = "fused_nn_softmax"
target datalayout = "e-m:e-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%0 = type { i8*, %1, i32, %2, i64*, i64*, i64 }
%1 = type { i32, i32 }
%2 = type { i8, i8, i16 }

@__tvm_main__ = weak local_unnamed_addr constant [17 x i8] c"fused_nn_softmax\00", align 1

; Function Attrs: nounwind
define dllexport i32 @fused_nn_softmax(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr #0 !dbg !5 {
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
  tail call fastcc void @fused_nn_softmax_compute_(i8* %9, i8* %11), !dbg !15
  ret i32 0, !dbg !15
}

; Function Attrs: noinline nounwind
define private fastcc void @fused_nn_softmax_compute_(i8* noalias nocapture readonly, i8* noalias nocapture) unnamed_addr #1 {
entry:
  %2 = bitcast i8* %0 to float*
  %3 = load float, float* %2, align 4, !tbaa !16
  %4 = fcmp olt float %3, 0xC7EFFFFFE0000000
  %5 = select i1 %4, float 0xC7EFFFFFE0000000, float %3
  %6 = getelementptr inbounds i8, i8* %0, i64 4
  %7 = bitcast i8* %6 to float*
  %8 = load float, float* %7, align 4, !tbaa !16
  %9 = fcmp ogt float %5, %8
  %10 = select i1 %9, float %5, float %8
  %11 = getelementptr inbounds i8, i8* %0, i64 8
  %12 = bitcast i8* %11 to float*
  %13 = load float, float* %12, align 4, !tbaa !16
  %14 = fcmp ogt float %10, %13
  %15 = select i1 %14, float %10, float %13
  %16 = getelementptr inbounds i8, i8* %0, i64 12
  %17 = bitcast i8* %16 to float*
  %18 = load float, float* %17, align 4, !tbaa !16
  %19 = fcmp ogt float %15, %18
  %20 = select i1 %19, float %15, float %18
  %21 = getelementptr inbounds i8, i8* %0, i64 16
  %22 = bitcast i8* %21 to float*
  %23 = load float, float* %22, align 4, !tbaa !16
  %24 = fcmp ogt float %20, %23
  %25 = select i1 %24, float %20, float %23
  %26 = getelementptr inbounds i8, i8* %0, i64 20
  %27 = bitcast i8* %26 to float*
  %28 = load float, float* %27, align 4, !tbaa !16
  %29 = fcmp ogt float %25, %28
  %30 = select i1 %29, float %25, float %28
  %31 = getelementptr inbounds i8, i8* %0, i64 24
  %32 = bitcast i8* %31 to float*
  %33 = load float, float* %32, align 4, !tbaa !16
  %34 = fcmp ogt float %30, %33
  %35 = select i1 %34, float %30, float %33
  %36 = getelementptr inbounds i8, i8* %0, i64 28
  %37 = bitcast i8* %36 to float*
  %38 = load float, float* %37, align 4, !tbaa !16
  %39 = fcmp ogt float %35, %38
  %40 = select i1 %39, float %35, float %38
  %41 = getelementptr inbounds i8, i8* %0, i64 32
  %42 = bitcast i8* %41 to float*
  %43 = load float, float* %42, align 4, !tbaa !16
  %44 = fcmp ogt float %40, %43
  %45 = select i1 %44, float %40, float %43
  %46 = getelementptr inbounds i8, i8* %0, i64 36
  %47 = bitcast i8* %46 to float*
  %48 = load float, float* %47, align 4, !tbaa !16
  %49 = fcmp ogt float %45, %48
  %50 = select i1 %49, float %45, float %48
  %51 = getelementptr inbounds i8, i8* %0, i64 40
  %52 = bitcast i8* %51 to float*
  %53 = load float, float* %52, align 4, !tbaa !16
  %54 = fcmp ogt float %50, %53
  %55 = select i1 %54, float %50, float %53
  %56 = getelementptr inbounds i8, i8* %0, i64 44
  %57 = bitcast i8* %56 to float*
  %58 = load float, float* %57, align 4, !tbaa !16
  %59 = fcmp ogt float %55, %58
  %60 = select i1 %59, float %55, float %58
  %61 = getelementptr inbounds i8, i8* %0, i64 48
  %62 = bitcast i8* %61 to float*
  %63 = load float, float* %62, align 4, !tbaa !16
  %64 = fcmp ogt float %60, %63
  %65 = select i1 %64, float %60, float %63
  %66 = getelementptr inbounds i8, i8* %0, i64 52
  %67 = bitcast i8* %66 to float*
  %68 = load float, float* %67, align 4, !tbaa !16
  %69 = fcmp ogt float %65, %68
  %70 = select i1 %69, float %65, float %68
  %71 = getelementptr inbounds i8, i8* %0, i64 56
  %72 = bitcast i8* %71 to float*
  %73 = load float, float* %72, align 4, !tbaa !16
  %74 = fcmp ogt float %70, %73
  %75 = select i1 %74, float %70, float %73
  %76 = getelementptr inbounds i8, i8* %0, i64 60
  %77 = bitcast i8* %76 to float*
  %78 = load float, float* %77, align 4, !tbaa !16
  %79 = fcmp ogt float %75, %78
  %80 = select i1 %79, float %75, float %78
  %81 = getelementptr inbounds i8, i8* %0, i64 64
  %82 = bitcast i8* %81 to float*
  %83 = load float, float* %82, align 4, !tbaa !16
  %84 = fcmp ogt float %80, %83
  %85 = select i1 %84, float %80, float %83
  %86 = getelementptr inbounds i8, i8* %0, i64 68
  %87 = bitcast i8* %86 to float*
  %88 = load float, float* %87, align 4, !tbaa !16
  %89 = fcmp ogt float %85, %88
  %90 = select i1 %89, float %85, float %88
  %91 = getelementptr inbounds i8, i8* %0, i64 72
  %92 = bitcast i8* %91 to float*
  %93 = load float, float* %92, align 4, !tbaa !16
  %94 = fcmp ogt float %90, %93
  %95 = select i1 %94, float %90, float %93
  %96 = getelementptr inbounds i8, i8* %0, i64 76
  %97 = bitcast i8* %96 to float*
  %98 = load float, float* %97, align 4, !tbaa !16
  %99 = fcmp ogt float %95, %98
  %100 = select i1 %99, float %95, float %98
  %101 = getelementptr inbounds i8, i8* %0, i64 80
  %102 = bitcast i8* %101 to float*
  %103 = load float, float* %102, align 4, !tbaa !16
  %104 = fcmp ogt float %100, %103
  %105 = select i1 %104, float %100, float %103
  %106 = getelementptr inbounds i8, i8* %0, i64 84
  %107 = bitcast i8* %106 to float*
  %108 = load float, float* %107, align 4, !tbaa !16
  %109 = fcmp ogt float %105, %108
  %110 = select i1 %109, float %105, float %108
  %111 = getelementptr inbounds i8, i8* %0, i64 88
  %112 = bitcast i8* %111 to float*
  %113 = load float, float* %112, align 4, !tbaa !16
  %114 = fcmp ogt float %110, %113
  %115 = select i1 %114, float %110, float %113
  %116 = getelementptr inbounds i8, i8* %0, i64 92
  %117 = bitcast i8* %116 to float*
  %118 = load float, float* %117, align 4, !tbaa !16
  %119 = fcmp ogt float %115, %118
  %120 = select i1 %119, float %115, float %118
  %121 = getelementptr inbounds i8, i8* %0, i64 96
  %122 = bitcast i8* %121 to float*
  %123 = load float, float* %122, align 4, !tbaa !16
  %124 = fcmp ogt float %120, %123
  %125 = select i1 %124, float %120, float %123
  %126 = getelementptr inbounds i8, i8* %0, i64 100
  %127 = bitcast i8* %126 to float*
  %128 = load float, float* %127, align 4, !tbaa !16
  %129 = fcmp ogt float %125, %128
  %130 = select i1 %129, float %125, float %128
  %131 = getelementptr inbounds i8, i8* %0, i64 104
  %132 = bitcast i8* %131 to float*
  %133 = load float, float* %132, align 4, !tbaa !16
  %134 = fcmp ogt float %130, %133
  %135 = select i1 %134, float %130, float %133
  %136 = getelementptr inbounds i8, i8* %0, i64 108
  %137 = bitcast i8* %136 to float*
  %138 = load float, float* %137, align 4, !tbaa !16
  %139 = fcmp ogt float %135, %138
  %140 = select i1 %139, float %135, float %138
  %141 = getelementptr inbounds i8, i8* %0, i64 112
  %142 = bitcast i8* %141 to float*
  %143 = load float, float* %142, align 4, !tbaa !16
  %144 = fcmp ogt float %140, %143
  %145 = select i1 %144, float %140, float %143
  %146 = getelementptr inbounds i8, i8* %0, i64 116
  %147 = bitcast i8* %146 to float*
  %148 = load float, float* %147, align 4, !tbaa !16
  %149 = fcmp ogt float %145, %148
  %150 = select i1 %149, float %145, float %148
  %151 = getelementptr inbounds i8, i8* %0, i64 120
  %152 = bitcast i8* %151 to float*
  %153 = load float, float* %152, align 4, !tbaa !16
  %154 = fcmp ogt float %150, %153
  %155 = select i1 %154, float %150, float %153
  %156 = getelementptr inbounds i8, i8* %0, i64 124
  %157 = bitcast i8* %156 to float*
  %158 = load float, float* %157, align 4, !tbaa !16
  %159 = fcmp ogt float %155, %158
  %160 = select i1 %159, float %155, float %158
  %161 = getelementptr inbounds i8, i8* %0, i64 128
  %162 = bitcast i8* %161 to float*
  %163 = load float, float* %162, align 4, !tbaa !16
  %164 = fcmp ogt float %160, %163
  %165 = select i1 %164, float %160, float %163
  %166 = getelementptr inbounds i8, i8* %0, i64 132
  %167 = bitcast i8* %166 to float*
  %168 = load float, float* %167, align 4, !tbaa !16
  %169 = fcmp ogt float %165, %168
  %170 = select i1 %169, float %165, float %168
  %171 = getelementptr inbounds i8, i8* %0, i64 136
  %172 = bitcast i8* %171 to float*
  %173 = load float, float* %172, align 4, !tbaa !16
  %174 = fcmp ogt float %170, %173
  %175 = select i1 %174, float %170, float %173
  %176 = getelementptr inbounds i8, i8* %0, i64 140
  %177 = bitcast i8* %176 to float*
  %178 = load float, float* %177, align 4, !tbaa !16
  %179 = fcmp ogt float %175, %178
  %180 = select i1 %179, float %175, float %178
  %181 = getelementptr inbounds i8, i8* %0, i64 144
  %182 = bitcast i8* %181 to float*
  %183 = load float, float* %182, align 4, !tbaa !16
  %184 = fcmp ogt float %180, %183
  %185 = select i1 %184, float %180, float %183
  %186 = getelementptr inbounds i8, i8* %0, i64 148
  %187 = bitcast i8* %186 to float*
  %188 = load float, float* %187, align 4, !tbaa !16
  %189 = fcmp ogt float %185, %188
  %190 = select i1 %189, float %185, float %188
  %191 = getelementptr inbounds i8, i8* %0, i64 152
  %192 = bitcast i8* %191 to float*
  %193 = load float, float* %192, align 4, !tbaa !16
  %194 = fcmp ogt float %190, %193
  %195 = select i1 %194, float %190, float %193
  %196 = getelementptr inbounds i8, i8* %0, i64 156
  %197 = bitcast i8* %196 to float*
  %198 = load float, float* %197, align 4, !tbaa !16
  %199 = fcmp ogt float %195, %198
  %200 = select i1 %199, float %195, float %198
  %201 = getelementptr inbounds i8, i8* %0, i64 160
  %202 = bitcast i8* %201 to float*
  %203 = load float, float* %202, align 4, !tbaa !16
  %204 = fcmp ogt float %200, %203
  %205 = select i1 %204, float %200, float %203
  %206 = getelementptr inbounds i8, i8* %0, i64 164
  %207 = bitcast i8* %206 to float*
  %208 = load float, float* %207, align 4, !tbaa !16
  %209 = fcmp ogt float %205, %208
  %210 = select i1 %209, float %205, float %208
  %211 = getelementptr inbounds i8, i8* %0, i64 168
  %212 = bitcast i8* %211 to float*
  %213 = load float, float* %212, align 4, !tbaa !16
  %214 = fcmp ogt float %210, %213
  %215 = select i1 %214, float %210, float %213
  %216 = getelementptr inbounds i8, i8* %0, i64 172
  %217 = bitcast i8* %216 to float*
  %218 = load float, float* %217, align 4, !tbaa !16
  %219 = fcmp ogt float %215, %218
  %220 = select i1 %219, float %215, float %218
  %221 = getelementptr inbounds i8, i8* %0, i64 176
  %222 = bitcast i8* %221 to float*
  %223 = load float, float* %222, align 4, !tbaa !16
  %224 = fcmp ogt float %220, %223
  %225 = select i1 %224, float %220, float %223
  %226 = getelementptr inbounds i8, i8* %0, i64 180
  %227 = bitcast i8* %226 to float*
  %228 = load float, float* %227, align 4, !tbaa !16
  %229 = fcmp ogt float %225, %228
  %230 = select i1 %229, float %225, float %228
  %231 = getelementptr inbounds i8, i8* %0, i64 184
  %232 = bitcast i8* %231 to float*
  %233 = load float, float* %232, align 4, !tbaa !16
  %234 = fcmp ogt float %230, %233
  %235 = select i1 %234, float %230, float %233
  %236 = getelementptr inbounds i8, i8* %0, i64 188
  %237 = bitcast i8* %236 to float*
  %238 = load float, float* %237, align 4, !tbaa !16
  %239 = fcmp ogt float %235, %238
  %240 = select i1 %239, float %235, float %238
  %241 = getelementptr inbounds i8, i8* %0, i64 192
  %242 = bitcast i8* %241 to float*
  %243 = load float, float* %242, align 4, !tbaa !16
  %244 = fcmp ogt float %240, %243
  %245 = select i1 %244, float %240, float %243
  %246 = getelementptr inbounds i8, i8* %0, i64 196
  %247 = bitcast i8* %246 to float*
  %248 = load float, float* %247, align 4, !tbaa !16
  %249 = fcmp ogt float %245, %248
  %250 = select i1 %249, float %245, float %248
  %251 = getelementptr inbounds i8, i8* %0, i64 200
  %252 = bitcast i8* %251 to float*
  %253 = load float, float* %252, align 4, !tbaa !16
  %254 = fcmp ogt float %250, %253
  %255 = select i1 %254, float %250, float %253
  %256 = getelementptr inbounds i8, i8* %0, i64 204
  %257 = bitcast i8* %256 to float*
  %258 = load float, float* %257, align 4, !tbaa !16
  %259 = fcmp ogt float %255, %258
  %260 = select i1 %259, float %255, float %258
  %261 = getelementptr inbounds i8, i8* %0, i64 208
  %262 = bitcast i8* %261 to float*
  %263 = load float, float* %262, align 4, !tbaa !16
  %264 = fcmp ogt float %260, %263
  %265 = select i1 %264, float %260, float %263
  %266 = getelementptr inbounds i8, i8* %0, i64 212
  %267 = bitcast i8* %266 to float*
  %268 = load float, float* %267, align 4, !tbaa !16
  %269 = fcmp ogt float %265, %268
  %270 = select i1 %269, float %265, float %268
  %271 = getelementptr inbounds i8, i8* %0, i64 216
  %272 = bitcast i8* %271 to float*
  %273 = load float, float* %272, align 4, !tbaa !16
  %274 = fcmp ogt float %270, %273
  %275 = select i1 %274, float %270, float %273
  %276 = getelementptr inbounds i8, i8* %0, i64 220
  %277 = bitcast i8* %276 to float*
  %278 = load float, float* %277, align 4, !tbaa !16
  %279 = fcmp ogt float %275, %278
  %280 = select i1 %279, float %275, float %278
  %281 = getelementptr inbounds i8, i8* %0, i64 224
  %282 = bitcast i8* %281 to float*
  %283 = load float, float* %282, align 4, !tbaa !16
  %284 = fcmp ogt float %280, %283
  %285 = select i1 %284, float %280, float %283
  %286 = getelementptr inbounds i8, i8* %0, i64 228
  %287 = bitcast i8* %286 to float*
  %288 = load float, float* %287, align 4, !tbaa !16
  %289 = fcmp ogt float %285, %288
  %290 = select i1 %289, float %285, float %288
  %291 = getelementptr inbounds i8, i8* %0, i64 232
  %292 = bitcast i8* %291 to float*
  %293 = load float, float* %292, align 4, !tbaa !16
  %294 = fcmp ogt float %290, %293
  %295 = select i1 %294, float %290, float %293
  %296 = getelementptr inbounds i8, i8* %0, i64 236
  %297 = bitcast i8* %296 to float*
  %298 = load float, float* %297, align 4, !tbaa !16
  %299 = fcmp ogt float %295, %298
  %300 = select i1 %299, float %295, float %298
  %301 = getelementptr inbounds i8, i8* %0, i64 240
  %302 = bitcast i8* %301 to float*
  %303 = load float, float* %302, align 4, !tbaa !16
  %304 = fcmp ogt float %300, %303
  %305 = select i1 %304, float %300, float %303
  %306 = getelementptr inbounds i8, i8* %0, i64 244
  %307 = bitcast i8* %306 to float*
  %308 = load float, float* %307, align 4, !tbaa !16
  %309 = fcmp ogt float %305, %308
  %310 = select i1 %309, float %305, float %308
  %311 = getelementptr inbounds i8, i8* %0, i64 248
  %312 = bitcast i8* %311 to float*
  %313 = load float, float* %312, align 4, !tbaa !16
  %314 = fcmp ogt float %310, %313
  %315 = select i1 %314, float %310, float %313
  %316 = getelementptr inbounds i8, i8* %0, i64 252
  %317 = bitcast i8* %316 to float*
  %318 = load float, float* %317, align 4, !tbaa !16
  %319 = fcmp ogt float %315, %318
  %320 = select i1 %319, float %315, float %318
  br label %for_body2

for_body2:                                        ; preds = %for_body2, %entry
  %indvars.iv4 = phi i64 [ 0, %entry ], [ %indvars.iv.next5, %for_body2 ]
  %.0111 = phi float [ 0.000000e+00, %entry ], [ %325, %for_body2 ]
  %321 = getelementptr inbounds float, float* %2, i64 %indvars.iv4
  %322 = load float, float* %321, align 4, !tbaa !16
  %323 = fsub float %322, %320
  %324 = tail call float @llvm.exp.f32(float %323)
  %325 = fadd float %.0111, %324
  %indvars.iv.next5 = add nuw nsw i64 %indvars.iv4, 1
  %exitcond6 = icmp eq i64 %indvars.iv.next5, 64
  br i1 %exitcond6, label %for_end3, label %for_body2, !prof !20

for_end3:                                         ; preds = %for_body2
  %broadcast.splatinsert10 = insertelement <4 x float> undef, float %320, i32 0
  %broadcast.splatinsert12 = insertelement <4 x float> undef, float %325, i32 0
  %326 = bitcast i8* %0 to <4 x float>*
  %.i0 = bitcast <4 x float>* %326 to float*
  %wide.load.i0 = load float, float* %.i0, align 4, !tbaa !16
  %.i1 = getelementptr float, float* %.i0, i32 1
  %wide.load.i1 = load float, float* %.i1, align 4, !tbaa !16
  %.i2 = getelementptr float, float* %.i0, i32 2
  %wide.load.i2 = load float, float* %.i2, align 4, !tbaa !16
  %.i3 = getelementptr float, float* %.i0, i32 3
  %wide.load.i3 = load float, float* %.i3, align 4, !tbaa !16
  %.i01 = fsub float %wide.load.i0, %320
  %.i12 = fsub float %wide.load.i1, %320
  %.i23 = fsub float %wide.load.i2, %320
  %.i34 = fsub float %wide.load.i3, %320
  %.i05 = call float @llvm.exp.f32(float %.i01)
  %.i16 = call float @llvm.exp.f32(float %.i12)
  %.i27 = call float @llvm.exp.f32(float %.i23)
  %.i38 = call float @llvm.exp.f32(float %.i34)
  %.i09 = fdiv float %.i05, %325
  %.i110 = fdiv float %.i16, %325
  %.i211 = fdiv float %.i27, %325
  %.i312 = fdiv float %.i38, %325
  %327 = bitcast i8* %1 to <4 x float>*
  %.i013 = bitcast <4 x float>* %327 to float*
  store float %.i09, float* %.i013, align 4, !tbaa !21
  %.i114 = getelementptr float, float* %.i013, i32 1
  store float %.i110, float* %.i114, align 4, !tbaa !21
  %.i215 = getelementptr float, float* %.i013, i32 2
  store float %.i211, float* %.i215, align 4, !tbaa !21
  %.i316 = getelementptr float, float* %.i013, i32 3
  store float %.i312, float* %.i316, align 4, !tbaa !21
  %328 = getelementptr inbounds i8, i8* %0, i64 16
  %329 = bitcast i8* %328 to <4 x float>*
  %.i017 = bitcast <4 x float>* %329 to float*
  %wide.load.1.i0 = load float, float* %.i017, align 4, !tbaa !16
  %.i118 = getelementptr float, float* %.i017, i32 1
  %wide.load.1.i1 = load float, float* %.i118, align 4, !tbaa !16
  %.i219 = getelementptr float, float* %.i017, i32 2
  %wide.load.1.i2 = load float, float* %.i219, align 4, !tbaa !16
  %.i320 = getelementptr float, float* %.i017, i32 3
  %wide.load.1.i3 = load float, float* %.i320, align 4, !tbaa !16
  %.i021 = fsub float %wide.load.1.i0, %320
  %.i122 = fsub float %wide.load.1.i1, %320
  %.i223 = fsub float %wide.load.1.i2, %320
  %.i324 = fsub float %wide.load.1.i3, %320
  %.i025 = call float @llvm.exp.f32(float %.i021)
  %.i126 = call float @llvm.exp.f32(float %.i122)
  %.i227 = call float @llvm.exp.f32(float %.i223)
  %.i328 = call float @llvm.exp.f32(float %.i324)
  %.i029 = fdiv float %.i025, %325
  %.i130 = fdiv float %.i126, %325
  %.i231 = fdiv float %.i227, %325
  %.i332 = fdiv float %.i328, %325
  %330 = getelementptr inbounds i8, i8* %1, i64 16
  %331 = bitcast i8* %330 to <4 x float>*
  %.i033 = bitcast <4 x float>* %331 to float*
  store float %.i029, float* %.i033, align 4, !tbaa !21
  %.i134 = getelementptr float, float* %.i033, i32 1
  store float %.i130, float* %.i134, align 4, !tbaa !21
  %.i235 = getelementptr float, float* %.i033, i32 2
  store float %.i231, float* %.i235, align 4, !tbaa !21
  %.i336 = getelementptr float, float* %.i033, i32 3
  store float %.i332, float* %.i336, align 4, !tbaa !21
  %332 = getelementptr inbounds i8, i8* %0, i64 32
  %333 = bitcast i8* %332 to <4 x float>*
  %.i037 = bitcast <4 x float>* %333 to float*
  %wide.load.2.i0 = load float, float* %.i037, align 4, !tbaa !16
  %.i138 = getelementptr float, float* %.i037, i32 1
  %wide.load.2.i1 = load float, float* %.i138, align 4, !tbaa !16
  %.i239 = getelementptr float, float* %.i037, i32 2
  %wide.load.2.i2 = load float, float* %.i239, align 4, !tbaa !16
  %.i340 = getelementptr float, float* %.i037, i32 3
  %wide.load.2.i3 = load float, float* %.i340, align 4, !tbaa !16
  %.i041 = fsub float %wide.load.2.i0, %320
  %.i142 = fsub float %wide.load.2.i1, %320
  %.i243 = fsub float %wide.load.2.i2, %320
  %.i344 = fsub float %wide.load.2.i3, %320
  %.i045 = call float @llvm.exp.f32(float %.i041)
  %.i146 = call float @llvm.exp.f32(float %.i142)
  %.i247 = call float @llvm.exp.f32(float %.i243)
  %.i348 = call float @llvm.exp.f32(float %.i344)
  %.i049 = fdiv float %.i045, %325
  %.i150 = fdiv float %.i146, %325
  %.i251 = fdiv float %.i247, %325
  %.i352 = fdiv float %.i348, %325
  %334 = getelementptr inbounds i8, i8* %1, i64 32
  %335 = bitcast i8* %334 to <4 x float>*
  %.i053 = bitcast <4 x float>* %335 to float*
  store float %.i049, float* %.i053, align 4, !tbaa !21
  %.i154 = getelementptr float, float* %.i053, i32 1
  store float %.i150, float* %.i154, align 4, !tbaa !21
  %.i255 = getelementptr float, float* %.i053, i32 2
  store float %.i251, float* %.i255, align 4, !tbaa !21
  %.i356 = getelementptr float, float* %.i053, i32 3
  store float %.i352, float* %.i356, align 4, !tbaa !21
  %336 = getelementptr inbounds i8, i8* %0, i64 48
  %337 = bitcast i8* %336 to <4 x float>*
  %.i057 = bitcast <4 x float>* %337 to float*
  %wide.load.3.i0 = load float, float* %.i057, align 4, !tbaa !16
  %.i158 = getelementptr float, float* %.i057, i32 1
  %wide.load.3.i1 = load float, float* %.i158, align 4, !tbaa !16
  %.i259 = getelementptr float, float* %.i057, i32 2
  %wide.load.3.i2 = load float, float* %.i259, align 4, !tbaa !16
  %.i360 = getelementptr float, float* %.i057, i32 3
  %wide.load.3.i3 = load float, float* %.i360, align 4, !tbaa !16
  %.i061 = fsub float %wide.load.3.i0, %320
  %.i162 = fsub float %wide.load.3.i1, %320
  %.i263 = fsub float %wide.load.3.i2, %320
  %.i364 = fsub float %wide.load.3.i3, %320
  %.i065 = call float @llvm.exp.f32(float %.i061)
  %.i166 = call float @llvm.exp.f32(float %.i162)
  %.i267 = call float @llvm.exp.f32(float %.i263)
  %.i368 = call float @llvm.exp.f32(float %.i364)
  %.i069 = fdiv float %.i065, %325
  %.i170 = fdiv float %.i166, %325
  %.i271 = fdiv float %.i267, %325
  %.i372 = fdiv float %.i368, %325
  %338 = getelementptr inbounds i8, i8* %1, i64 48
  %339 = bitcast i8* %338 to <4 x float>*
  %.i073 = bitcast <4 x float>* %339 to float*
  store float %.i069, float* %.i073, align 4, !tbaa !21
  %.i174 = getelementptr float, float* %.i073, i32 1
  store float %.i170, float* %.i174, align 4, !tbaa !21
  %.i275 = getelementptr float, float* %.i073, i32 2
  store float %.i271, float* %.i275, align 4, !tbaa !21
  %.i376 = getelementptr float, float* %.i073, i32 3
  store float %.i372, float* %.i376, align 4, !tbaa !21
  %340 = getelementptr inbounds i8, i8* %0, i64 64
  %341 = bitcast i8* %340 to <4 x float>*
  %.i077 = bitcast <4 x float>* %341 to float*
  %wide.load.4.i0 = load float, float* %.i077, align 4, !tbaa !16
  %.i178 = getelementptr float, float* %.i077, i32 1
  %wide.load.4.i1 = load float, float* %.i178, align 4, !tbaa !16
  %.i279 = getelementptr float, float* %.i077, i32 2
  %wide.load.4.i2 = load float, float* %.i279, align 4, !tbaa !16
  %.i380 = getelementptr float, float* %.i077, i32 3
  %wide.load.4.i3 = load float, float* %.i380, align 4, !tbaa !16
  %.i081 = fsub float %wide.load.4.i0, %320
  %.i182 = fsub float %wide.load.4.i1, %320
  %.i283 = fsub float %wide.load.4.i2, %320
  %.i384 = fsub float %wide.load.4.i3, %320
  %.i085 = call float @llvm.exp.f32(float %.i081)
  %.i186 = call float @llvm.exp.f32(float %.i182)
  %.i287 = call float @llvm.exp.f32(float %.i283)
  %.i388 = call float @llvm.exp.f32(float %.i384)
  %.i089 = fdiv float %.i085, %325
  %.i190 = fdiv float %.i186, %325
  %.i291 = fdiv float %.i287, %325
  %.i392 = fdiv float %.i388, %325
  %342 = getelementptr inbounds i8, i8* %1, i64 64
  %343 = bitcast i8* %342 to <4 x float>*
  %.i093 = bitcast <4 x float>* %343 to float*
  store float %.i089, float* %.i093, align 4, !tbaa !21
  %.i194 = getelementptr float, float* %.i093, i32 1
  store float %.i190, float* %.i194, align 4, !tbaa !21
  %.i295 = getelementptr float, float* %.i093, i32 2
  store float %.i291, float* %.i295, align 4, !tbaa !21
  %.i396 = getelementptr float, float* %.i093, i32 3
  store float %.i392, float* %.i396, align 4, !tbaa !21
  %344 = getelementptr inbounds i8, i8* %0, i64 80
  %345 = bitcast i8* %344 to <4 x float>*
  %.i097 = bitcast <4 x float>* %345 to float*
  %wide.load.5.i0 = load float, float* %.i097, align 4, !tbaa !16
  %.i198 = getelementptr float, float* %.i097, i32 1
  %wide.load.5.i1 = load float, float* %.i198, align 4, !tbaa !16
  %.i299 = getelementptr float, float* %.i097, i32 2
  %wide.load.5.i2 = load float, float* %.i299, align 4, !tbaa !16
  %.i3100 = getelementptr float, float* %.i097, i32 3
  %wide.load.5.i3 = load float, float* %.i3100, align 4, !tbaa !16
  %.i0101 = fsub float %wide.load.5.i0, %320
  %.i1102 = fsub float %wide.load.5.i1, %320
  %.i2103 = fsub float %wide.load.5.i2, %320
  %.i3104 = fsub float %wide.load.5.i3, %320
  %.i0105 = call float @llvm.exp.f32(float %.i0101)
  %.i1106 = call float @llvm.exp.f32(float %.i1102)
  %.i2107 = call float @llvm.exp.f32(float %.i2103)
  %.i3108 = call float @llvm.exp.f32(float %.i3104)
  %.i0109 = fdiv float %.i0105, %325
  %.i1110 = fdiv float %.i1106, %325
  %.i2111 = fdiv float %.i2107, %325
  %.i3112 = fdiv float %.i3108, %325
  %346 = getelementptr inbounds i8, i8* %1, i64 80
  %347 = bitcast i8* %346 to <4 x float>*
  %.i0113 = bitcast <4 x float>* %347 to float*
  store float %.i0109, float* %.i0113, align 4, !tbaa !21
  %.i1114 = getelementptr float, float* %.i0113, i32 1
  store float %.i1110, float* %.i1114, align 4, !tbaa !21
  %.i2115 = getelementptr float, float* %.i0113, i32 2
  store float %.i2111, float* %.i2115, align 4, !tbaa !21
  %.i3116 = getelementptr float, float* %.i0113, i32 3
  store float %.i3112, float* %.i3116, align 4, !tbaa !21
  %348 = getelementptr inbounds i8, i8* %0, i64 96
  %349 = bitcast i8* %348 to <4 x float>*
  %.i0117 = bitcast <4 x float>* %349 to float*
  %wide.load.6.i0 = load float, float* %.i0117, align 4, !tbaa !16
  %.i1118 = getelementptr float, float* %.i0117, i32 1
  %wide.load.6.i1 = load float, float* %.i1118, align 4, !tbaa !16
  %.i2119 = getelementptr float, float* %.i0117, i32 2
  %wide.load.6.i2 = load float, float* %.i2119, align 4, !tbaa !16
  %.i3120 = getelementptr float, float* %.i0117, i32 3
  %wide.load.6.i3 = load float, float* %.i3120, align 4, !tbaa !16
  %.i0121 = fsub float %wide.load.6.i0, %320
  %.i1122 = fsub float %wide.load.6.i1, %320
  %.i2123 = fsub float %wide.load.6.i2, %320
  %.i3124 = fsub float %wide.load.6.i3, %320
  %.i0125 = call float @llvm.exp.f32(float %.i0121)
  %.i1126 = call float @llvm.exp.f32(float %.i1122)
  %.i2127 = call float @llvm.exp.f32(float %.i2123)
  %.i3128 = call float @llvm.exp.f32(float %.i3124)
  %.i0129 = fdiv float %.i0125, %325
  %.i1130 = fdiv float %.i1126, %325
  %.i2131 = fdiv float %.i2127, %325
  %.i3132 = fdiv float %.i3128, %325
  %350 = getelementptr inbounds i8, i8* %1, i64 96
  %351 = bitcast i8* %350 to <4 x float>*
  %.i0133 = bitcast <4 x float>* %351 to float*
  store float %.i0129, float* %.i0133, align 4, !tbaa !21
  %.i1134 = getelementptr float, float* %.i0133, i32 1
  store float %.i1130, float* %.i1134, align 4, !tbaa !21
  %.i2135 = getelementptr float, float* %.i0133, i32 2
  store float %.i2131, float* %.i2135, align 4, !tbaa !21
  %.i3136 = getelementptr float, float* %.i0133, i32 3
  store float %.i3132, float* %.i3136, align 4, !tbaa !21
  %352 = getelementptr inbounds i8, i8* %0, i64 112
  %353 = bitcast i8* %352 to <4 x float>*
  %.i0137 = bitcast <4 x float>* %353 to float*
  %wide.load.7.i0 = load float, float* %.i0137, align 4, !tbaa !16
  %.i1138 = getelementptr float, float* %.i0137, i32 1
  %wide.load.7.i1 = load float, float* %.i1138, align 4, !tbaa !16
  %.i2139 = getelementptr float, float* %.i0137, i32 2
  %wide.load.7.i2 = load float, float* %.i2139, align 4, !tbaa !16
  %.i3140 = getelementptr float, float* %.i0137, i32 3
  %wide.load.7.i3 = load float, float* %.i3140, align 4, !tbaa !16
  %.i0141 = fsub float %wide.load.7.i0, %320
  %.i1142 = fsub float %wide.load.7.i1, %320
  %.i2143 = fsub float %wide.load.7.i2, %320
  %.i3144 = fsub float %wide.load.7.i3, %320
  %.i0145 = call float @llvm.exp.f32(float %.i0141)
  %.i1146 = call float @llvm.exp.f32(float %.i1142)
  %.i2147 = call float @llvm.exp.f32(float %.i2143)
  %.i3148 = call float @llvm.exp.f32(float %.i3144)
  %.i0149 = fdiv float %.i0145, %325
  %.i1150 = fdiv float %.i1146, %325
  %.i2151 = fdiv float %.i2147, %325
  %.i3152 = fdiv float %.i3148, %325
  %354 = getelementptr inbounds i8, i8* %1, i64 112
  %355 = bitcast i8* %354 to <4 x float>*
  %.i0153 = bitcast <4 x float>* %355 to float*
  store float %.i0149, float* %.i0153, align 4, !tbaa !21
  %.i1154 = getelementptr float, float* %.i0153, i32 1
  store float %.i1150, float* %.i1154, align 4, !tbaa !21
  %.i2155 = getelementptr float, float* %.i0153, i32 2
  store float %.i2151, float* %.i2155, align 4, !tbaa !21
  %.i3156 = getelementptr float, float* %.i0153, i32 3
  store float %.i3152, float* %.i3156, align 4, !tbaa !21
  %356 = getelementptr inbounds i8, i8* %0, i64 128
  %357 = bitcast i8* %356 to <4 x float>*
  %.i0157 = bitcast <4 x float>* %357 to float*
  %wide.load.8.i0 = load float, float* %.i0157, align 4, !tbaa !16
  %.i1158 = getelementptr float, float* %.i0157, i32 1
  %wide.load.8.i1 = load float, float* %.i1158, align 4, !tbaa !16
  %.i2159 = getelementptr float, float* %.i0157, i32 2
  %wide.load.8.i2 = load float, float* %.i2159, align 4, !tbaa !16
  %.i3160 = getelementptr float, float* %.i0157, i32 3
  %wide.load.8.i3 = load float, float* %.i3160, align 4, !tbaa !16
  %.i0161 = fsub float %wide.load.8.i0, %320
  %.i1162 = fsub float %wide.load.8.i1, %320
  %.i2163 = fsub float %wide.load.8.i2, %320
  %.i3164 = fsub float %wide.load.8.i3, %320
  %.i0165 = call float @llvm.exp.f32(float %.i0161)
  %.i1166 = call float @llvm.exp.f32(float %.i1162)
  %.i2167 = call float @llvm.exp.f32(float %.i2163)
  %.i3168 = call float @llvm.exp.f32(float %.i3164)
  %.i0169 = fdiv float %.i0165, %325
  %.i1170 = fdiv float %.i1166, %325
  %.i2171 = fdiv float %.i2167, %325
  %.i3172 = fdiv float %.i3168, %325
  %358 = getelementptr inbounds i8, i8* %1, i64 128
  %359 = bitcast i8* %358 to <4 x float>*
  %.i0173 = bitcast <4 x float>* %359 to float*
  store float %.i0169, float* %.i0173, align 4, !tbaa !21
  %.i1174 = getelementptr float, float* %.i0173, i32 1
  store float %.i1170, float* %.i1174, align 4, !tbaa !21
  %.i2175 = getelementptr float, float* %.i0173, i32 2
  store float %.i2171, float* %.i2175, align 4, !tbaa !21
  %.i3176 = getelementptr float, float* %.i0173, i32 3
  store float %.i3172, float* %.i3176, align 4, !tbaa !21
  %360 = getelementptr inbounds i8, i8* %0, i64 144
  %361 = bitcast i8* %360 to <4 x float>*
  %.i0177 = bitcast <4 x float>* %361 to float*
  %wide.load.9.i0 = load float, float* %.i0177, align 4, !tbaa !16
  %.i1178 = getelementptr float, float* %.i0177, i32 1
  %wide.load.9.i1 = load float, float* %.i1178, align 4, !tbaa !16
  %.i2179 = getelementptr float, float* %.i0177, i32 2
  %wide.load.9.i2 = load float, float* %.i2179, align 4, !tbaa !16
  %.i3180 = getelementptr float, float* %.i0177, i32 3
  %wide.load.9.i3 = load float, float* %.i3180, align 4, !tbaa !16
  %.i0181 = fsub float %wide.load.9.i0, %320
  %.i1182 = fsub float %wide.load.9.i1, %320
  %.i2183 = fsub float %wide.load.9.i2, %320
  %.i3184 = fsub float %wide.load.9.i3, %320
  %.i0185 = call float @llvm.exp.f32(float %.i0181)
  %.i1186 = call float @llvm.exp.f32(float %.i1182)
  %.i2187 = call float @llvm.exp.f32(float %.i2183)
  %.i3188 = call float @llvm.exp.f32(float %.i3184)
  %.i0189 = fdiv float %.i0185, %325
  %.i1190 = fdiv float %.i1186, %325
  %.i2191 = fdiv float %.i2187, %325
  %.i3192 = fdiv float %.i3188, %325
  %362 = getelementptr inbounds i8, i8* %1, i64 144
  %363 = bitcast i8* %362 to <4 x float>*
  %.i0193 = bitcast <4 x float>* %363 to float*
  store float %.i0189, float* %.i0193, align 4, !tbaa !21
  %.i1194 = getelementptr float, float* %.i0193, i32 1
  store float %.i1190, float* %.i1194, align 4, !tbaa !21
  %.i2195 = getelementptr float, float* %.i0193, i32 2
  store float %.i2191, float* %.i2195, align 4, !tbaa !21
  %.i3196 = getelementptr float, float* %.i0193, i32 3
  store float %.i3192, float* %.i3196, align 4, !tbaa !21
  %364 = getelementptr inbounds i8, i8* %0, i64 160
  %365 = bitcast i8* %364 to <4 x float>*
  %.i0197 = bitcast <4 x float>* %365 to float*
  %wide.load.10.i0 = load float, float* %.i0197, align 4, !tbaa !16
  %.i1198 = getelementptr float, float* %.i0197, i32 1
  %wide.load.10.i1 = load float, float* %.i1198, align 4, !tbaa !16
  %.i2199 = getelementptr float, float* %.i0197, i32 2
  %wide.load.10.i2 = load float, float* %.i2199, align 4, !tbaa !16
  %.i3200 = getelementptr float, float* %.i0197, i32 3
  %wide.load.10.i3 = load float, float* %.i3200, align 4, !tbaa !16
  %.i0201 = fsub float %wide.load.10.i0, %320
  %.i1202 = fsub float %wide.load.10.i1, %320
  %.i2203 = fsub float %wide.load.10.i2, %320
  %.i3204 = fsub float %wide.load.10.i3, %320
  %.i0205 = call float @llvm.exp.f32(float %.i0201)
  %.i1206 = call float @llvm.exp.f32(float %.i1202)
  %.i2207 = call float @llvm.exp.f32(float %.i2203)
  %.i3208 = call float @llvm.exp.f32(float %.i3204)
  %.i0209 = fdiv float %.i0205, %325
  %.i1210 = fdiv float %.i1206, %325
  %.i2211 = fdiv float %.i2207, %325
  %.i3212 = fdiv float %.i3208, %325
  %366 = getelementptr inbounds i8, i8* %1, i64 160
  %367 = bitcast i8* %366 to <4 x float>*
  %.i0213 = bitcast <4 x float>* %367 to float*
  store float %.i0209, float* %.i0213, align 4, !tbaa !21
  %.i1214 = getelementptr float, float* %.i0213, i32 1
  store float %.i1210, float* %.i1214, align 4, !tbaa !21
  %.i2215 = getelementptr float, float* %.i0213, i32 2
  store float %.i2211, float* %.i2215, align 4, !tbaa !21
  %.i3216 = getelementptr float, float* %.i0213, i32 3
  store float %.i3212, float* %.i3216, align 4, !tbaa !21
  %368 = getelementptr inbounds i8, i8* %0, i64 176
  %369 = bitcast i8* %368 to <4 x float>*
  %.i0217 = bitcast <4 x float>* %369 to float*
  %wide.load.11.i0 = load float, float* %.i0217, align 4, !tbaa !16
  %.i1218 = getelementptr float, float* %.i0217, i32 1
  %wide.load.11.i1 = load float, float* %.i1218, align 4, !tbaa !16
  %.i2219 = getelementptr float, float* %.i0217, i32 2
  %wide.load.11.i2 = load float, float* %.i2219, align 4, !tbaa !16
  %.i3220 = getelementptr float, float* %.i0217, i32 3
  %wide.load.11.i3 = load float, float* %.i3220, align 4, !tbaa !16
  %.i0221 = fsub float %wide.load.11.i0, %320
  %.i1222 = fsub float %wide.load.11.i1, %320
  %.i2223 = fsub float %wide.load.11.i2, %320
  %.i3224 = fsub float %wide.load.11.i3, %320
  %.i0225 = call float @llvm.exp.f32(float %.i0221)
  %.i1226 = call float @llvm.exp.f32(float %.i1222)
  %.i2227 = call float @llvm.exp.f32(float %.i2223)
  %.i3228 = call float @llvm.exp.f32(float %.i3224)
  %.i0229 = fdiv float %.i0225, %325
  %.i1230 = fdiv float %.i1226, %325
  %.i2231 = fdiv float %.i2227, %325
  %.i3232 = fdiv float %.i3228, %325
  %370 = getelementptr inbounds i8, i8* %1, i64 176
  %371 = bitcast i8* %370 to <4 x float>*
  %.i0233 = bitcast <4 x float>* %371 to float*
  store float %.i0229, float* %.i0233, align 4, !tbaa !21
  %.i1234 = getelementptr float, float* %.i0233, i32 1
  store float %.i1230, float* %.i1234, align 4, !tbaa !21
  %.i2235 = getelementptr float, float* %.i0233, i32 2
  store float %.i2231, float* %.i2235, align 4, !tbaa !21
  %.i3236 = getelementptr float, float* %.i0233, i32 3
  store float %.i3232, float* %.i3236, align 4, !tbaa !21
  %372 = getelementptr inbounds i8, i8* %0, i64 192
  %373 = bitcast i8* %372 to <4 x float>*
  %.i0237 = bitcast <4 x float>* %373 to float*
  %wide.load.12.i0 = load float, float* %.i0237, align 4, !tbaa !16
  %.i1238 = getelementptr float, float* %.i0237, i32 1
  %wide.load.12.i1 = load float, float* %.i1238, align 4, !tbaa !16
  %.i2239 = getelementptr float, float* %.i0237, i32 2
  %wide.load.12.i2 = load float, float* %.i2239, align 4, !tbaa !16
  %.i3240 = getelementptr float, float* %.i0237, i32 3
  %wide.load.12.i3 = load float, float* %.i3240, align 4, !tbaa !16
  %.i0241 = fsub float %wide.load.12.i0, %320
  %.i1242 = fsub float %wide.load.12.i1, %320
  %.i2243 = fsub float %wide.load.12.i2, %320
  %.i3244 = fsub float %wide.load.12.i3, %320
  %.i0245 = call float @llvm.exp.f32(float %.i0241)
  %.i1246 = call float @llvm.exp.f32(float %.i1242)
  %.i2247 = call float @llvm.exp.f32(float %.i2243)
  %.i3248 = call float @llvm.exp.f32(float %.i3244)
  %.i0249 = fdiv float %.i0245, %325
  %.i1250 = fdiv float %.i1246, %325
  %.i2251 = fdiv float %.i2247, %325
  %.i3252 = fdiv float %.i3248, %325
  %374 = getelementptr inbounds i8, i8* %1, i64 192
  %375 = bitcast i8* %374 to <4 x float>*
  %.i0253 = bitcast <4 x float>* %375 to float*
  store float %.i0249, float* %.i0253, align 4, !tbaa !21
  %.i1254 = getelementptr float, float* %.i0253, i32 1
  store float %.i1250, float* %.i1254, align 4, !tbaa !21
  %.i2255 = getelementptr float, float* %.i0253, i32 2
  store float %.i2251, float* %.i2255, align 4, !tbaa !21
  %.i3256 = getelementptr float, float* %.i0253, i32 3
  store float %.i3252, float* %.i3256, align 4, !tbaa !21
  %376 = getelementptr inbounds i8, i8* %0, i64 208
  %377 = bitcast i8* %376 to <4 x float>*
  %.i0257 = bitcast <4 x float>* %377 to float*
  %wide.load.13.i0 = load float, float* %.i0257, align 4, !tbaa !16
  %.i1258 = getelementptr float, float* %.i0257, i32 1
  %wide.load.13.i1 = load float, float* %.i1258, align 4, !tbaa !16
  %.i2259 = getelementptr float, float* %.i0257, i32 2
  %wide.load.13.i2 = load float, float* %.i2259, align 4, !tbaa !16
  %.i3260 = getelementptr float, float* %.i0257, i32 3
  %wide.load.13.i3 = load float, float* %.i3260, align 4, !tbaa !16
  %.i0261 = fsub float %wide.load.13.i0, %320
  %.i1262 = fsub float %wide.load.13.i1, %320
  %.i2263 = fsub float %wide.load.13.i2, %320
  %.i3264 = fsub float %wide.load.13.i3, %320
  %.i0265 = call float @llvm.exp.f32(float %.i0261)
  %.i1266 = call float @llvm.exp.f32(float %.i1262)
  %.i2267 = call float @llvm.exp.f32(float %.i2263)
  %.i3268 = call float @llvm.exp.f32(float %.i3264)
  %.i0269 = fdiv float %.i0265, %325
  %.i1270 = fdiv float %.i1266, %325
  %.i2271 = fdiv float %.i2267, %325
  %.i3272 = fdiv float %.i3268, %325
  %378 = getelementptr inbounds i8, i8* %1, i64 208
  %379 = bitcast i8* %378 to <4 x float>*
  %.i0273 = bitcast <4 x float>* %379 to float*
  store float %.i0269, float* %.i0273, align 4, !tbaa !21
  %.i1274 = getelementptr float, float* %.i0273, i32 1
  store float %.i1270, float* %.i1274, align 4, !tbaa !21
  %.i2275 = getelementptr float, float* %.i0273, i32 2
  store float %.i2271, float* %.i2275, align 4, !tbaa !21
  %.i3276 = getelementptr float, float* %.i0273, i32 3
  store float %.i3272, float* %.i3276, align 4, !tbaa !21
  %380 = getelementptr inbounds i8, i8* %0, i64 224
  %381 = bitcast i8* %380 to <4 x float>*
  %.i0277 = bitcast <4 x float>* %381 to float*
  %wide.load.14.i0 = load float, float* %.i0277, align 4, !tbaa !16
  %.i1278 = getelementptr float, float* %.i0277, i32 1
  %wide.load.14.i1 = load float, float* %.i1278, align 4, !tbaa !16
  %.i2279 = getelementptr float, float* %.i0277, i32 2
  %wide.load.14.i2 = load float, float* %.i2279, align 4, !tbaa !16
  %.i3280 = getelementptr float, float* %.i0277, i32 3
  %wide.load.14.i3 = load float, float* %.i3280, align 4, !tbaa !16
  %.i0281 = fsub float %wide.load.14.i0, %320
  %.i1282 = fsub float %wide.load.14.i1, %320
  %.i2283 = fsub float %wide.load.14.i2, %320
  %.i3284 = fsub float %wide.load.14.i3, %320
  %.i0285 = call float @llvm.exp.f32(float %.i0281)
  %.i1286 = call float @llvm.exp.f32(float %.i1282)
  %.i2287 = call float @llvm.exp.f32(float %.i2283)
  %.i3288 = call float @llvm.exp.f32(float %.i3284)
  %.i0289 = fdiv float %.i0285, %325
  %.i1290 = fdiv float %.i1286, %325
  %.i2291 = fdiv float %.i2287, %325
  %.i3292 = fdiv float %.i3288, %325
  %382 = getelementptr inbounds i8, i8* %1, i64 224
  %383 = bitcast i8* %382 to <4 x float>*
  %.i0293 = bitcast <4 x float>* %383 to float*
  store float %.i0289, float* %.i0293, align 4, !tbaa !21
  %.i1294 = getelementptr float, float* %.i0293, i32 1
  store float %.i1290, float* %.i1294, align 4, !tbaa !21
  %.i2295 = getelementptr float, float* %.i0293, i32 2
  store float %.i2291, float* %.i2295, align 4, !tbaa !21
  %.i3296 = getelementptr float, float* %.i0293, i32 3
  store float %.i3292, float* %.i3296, align 4, !tbaa !21
  %384 = getelementptr inbounds i8, i8* %0, i64 240
  %385 = bitcast i8* %384 to <4 x float>*
  %.i0297 = bitcast <4 x float>* %385 to float*
  %wide.load.15.i0 = load float, float* %.i0297, align 4, !tbaa !16
  %.i1298 = getelementptr float, float* %.i0297, i32 1
  %wide.load.15.i1 = load float, float* %.i1298, align 4, !tbaa !16
  %.i2299 = getelementptr float, float* %.i0297, i32 2
  %wide.load.15.i2 = load float, float* %.i2299, align 4, !tbaa !16
  %.i3300 = getelementptr float, float* %.i0297, i32 3
  %wide.load.15.i3 = load float, float* %.i3300, align 4, !tbaa !16
  %.i0301 = fsub float %wide.load.15.i0, %320
  %.i1302 = fsub float %wide.load.15.i1, %320
  %.i2303 = fsub float %wide.load.15.i2, %320
  %.i3304 = fsub float %wide.load.15.i3, %320
  %.i0305 = call float @llvm.exp.f32(float %.i0301)
  %.i1306 = call float @llvm.exp.f32(float %.i1302)
  %.i2307 = call float @llvm.exp.f32(float %.i2303)
  %.i3308 = call float @llvm.exp.f32(float %.i3304)
  %.i0309 = fdiv float %.i0305, %325
  %.i1310 = fdiv float %.i1306, %325
  %.i2311 = fdiv float %.i2307, %325
  %.i3312 = fdiv float %.i3308, %325
  %386 = getelementptr inbounds i8, i8* %1, i64 240
  %387 = bitcast i8* %386 to <4 x float>*
  %.i0313 = bitcast <4 x float>* %387 to float*
  store float %.i0309, float* %.i0313, align 4, !tbaa !21
  %.i1314 = getelementptr float, float* %.i0313, i32 1
  store float %.i1310, float* %.i1314, align 4, !tbaa !21
  %.i2315 = getelementptr float, float* %.i0313, i32 2
  store float %.i2311, float* %.i2315, align 4, !tbaa !21
  %.i3316 = getelementptr float, float* %.i0313, i32 3
  store float %.i3312, float* %.i3316, align 4, !tbaa !21
  ret void
}

; Function Attrs: nounwind readnone speculatable
declare float @llvm.exp.f32(float) #2

; Function Attrs: nounwind readnone speculatable
declare void @llvm.dbg.value(metadata, metadata, metadata) #2

; Function Attrs: nounwind readnone speculatable
declare <4 x float> @llvm.exp.v4f32(<4 x float>) #2

attributes #0 = { nounwind }
attributes #1 = { noinline nounwind }
attributes #2 = { nounwind readnone speculatable }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!3, !4}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "TVM", isOptimized: false, runtimeVersion: 0, emissionKind: FullDebug, enums: !2, dwoId: 1)
!1 = !DIFile(filename: "model.tvm", directory: "/tmp/")
!2 = !{}
!3 = !{i32 2, !"tvm_target", !"llvm"}
!4 = !{i32 4, !"Debug Info Version", i32 3}
!5 = distinct !DISubprogram(name: "fused_nn_softmax", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !11)
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
!18 = !{!"0x55cf235552e0", !19, i64 0}
!19 = !{!"tvm-tbaa"}
!20 = !{!"branch_weights", i32 1, i32 1048576}
!21 = !{!22, !22, i64 0}
!22 = !{!"float32", !23, i64 0}
!23 = !{!"0x55cf2354c480", !19, i64 0}
