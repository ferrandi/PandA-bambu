; ModuleID = 'fused_layout_transform_1'
source_filename = "fused_layout_transform_1"
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
@__tvm_main__ = weak local_unnamed_addr constant [25 x i8] c"fused_layout_transform_1\00", align 1

define dllexport i32 @fused_layout_transform_1(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !5 {
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
  %12 = tail call fastcc i32 @fused_layout_transform_1_compute_(i8* %11, i8* %9), !dbg !15
  ret i32 %12, !dbg !15
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_1_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
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
  %10 = add nsw i32 %9, 4
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 5
  %15 = select i1 %14, i32 %13, i32 5
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 5
  %18 = select i1 %17, i32 %16, i32 5
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
  %24 = trunc i64 %indvars.iv7 to i32
  %25 = shl i32 %24, 10
  %26 = sext i32 %25 to i64
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_body2, %for_body
  %indvars.iv = phi i64 [ 0, %for_body ], [ %indvars.iv.next, %for_body2 ]
  %27 = shl i64 %indvars.iv, 5
  %28 = add nsw i64 %27, %26
  %29 = trunc i64 %indvars.iv to i32
  %30 = mul i32 %29, 160
  %31 = add i32 %30, %24
  %32 = sext i32 %31 to i64
  %33 = getelementptr inbounds float, float* %7, i64 %32
  %34 = bitcast float* %33 to i32*
  %35 = load i32, i32* %34, align 4, !tbaa !20
  %36 = getelementptr inbounds float, float* %4, i64 %28
  %37 = bitcast float* %36 to i32*
  store i32 %35, i32* %37, align 4, !tbaa !23
  %38 = or i64 %28, 1
  %39 = add i32 %31, 5
  %40 = sext i32 %39 to i64
  %41 = getelementptr inbounds float, float* %7, i64 %40
  %42 = bitcast float* %41 to i32*
  %43 = load i32, i32* %42, align 4, !tbaa !20
  %44 = getelementptr inbounds float, float* %4, i64 %38
  %45 = bitcast float* %44 to i32*
  store i32 %43, i32* %45, align 4, !tbaa !23
  %46 = or i64 %28, 2
  %47 = add i32 %31, 10
  %48 = sext i32 %47 to i64
  %49 = getelementptr inbounds float, float* %7, i64 %48
  %50 = bitcast float* %49 to i32*
  %51 = load i32, i32* %50, align 4, !tbaa !20
  %52 = getelementptr inbounds float, float* %4, i64 %46
  %53 = bitcast float* %52 to i32*
  store i32 %51, i32* %53, align 4, !tbaa !23
  %54 = or i64 %28, 3
  %55 = add i32 %31, 15
  %56 = sext i32 %55 to i64
  %57 = getelementptr inbounds float, float* %7, i64 %56
  %58 = bitcast float* %57 to i32*
  %59 = load i32, i32* %58, align 4, !tbaa !20
  %60 = getelementptr inbounds float, float* %4, i64 %54
  %61 = bitcast float* %60 to i32*
  store i32 %59, i32* %61, align 4, !tbaa !23
  %62 = or i64 %28, 4
  %63 = add i32 %31, 20
  %64 = sext i32 %63 to i64
  %65 = getelementptr inbounds float, float* %7, i64 %64
  %66 = bitcast float* %65 to i32*
  %67 = load i32, i32* %66, align 4, !tbaa !20
  %68 = getelementptr inbounds float, float* %4, i64 %62
  %69 = bitcast float* %68 to i32*
  store i32 %67, i32* %69, align 4, !tbaa !23
  %70 = or i64 %28, 5
  %71 = add i32 %31, 25
  %72 = sext i32 %71 to i64
  %73 = getelementptr inbounds float, float* %7, i64 %72
  %74 = bitcast float* %73 to i32*
  %75 = load i32, i32* %74, align 4, !tbaa !20
  %76 = getelementptr inbounds float, float* %4, i64 %70
  %77 = bitcast float* %76 to i32*
  store i32 %75, i32* %77, align 4, !tbaa !23
  %78 = or i64 %28, 6
  %79 = add i32 %31, 30
  %80 = sext i32 %79 to i64
  %81 = getelementptr inbounds float, float* %7, i64 %80
  %82 = bitcast float* %81 to i32*
  %83 = load i32, i32* %82, align 4, !tbaa !20
  %84 = getelementptr inbounds float, float* %4, i64 %78
  %85 = bitcast float* %84 to i32*
  store i32 %83, i32* %85, align 4, !tbaa !23
  %86 = or i64 %28, 7
  %87 = add i32 %31, 35
  %88 = sext i32 %87 to i64
  %89 = getelementptr inbounds float, float* %7, i64 %88
  %90 = bitcast float* %89 to i32*
  %91 = load i32, i32* %90, align 4, !tbaa !20
  %92 = getelementptr inbounds float, float* %4, i64 %86
  %93 = bitcast float* %92 to i32*
  store i32 %91, i32* %93, align 4, !tbaa !23
  %94 = or i64 %28, 8
  %95 = add i32 %31, 40
  %96 = sext i32 %95 to i64
  %97 = getelementptr inbounds float, float* %7, i64 %96
  %98 = bitcast float* %97 to i32*
  %99 = load i32, i32* %98, align 4, !tbaa !20
  %100 = getelementptr inbounds float, float* %4, i64 %94
  %101 = bitcast float* %100 to i32*
  store i32 %99, i32* %101, align 4, !tbaa !23
  %102 = or i64 %28, 9
  %103 = add i32 %31, 45
  %104 = sext i32 %103 to i64
  %105 = getelementptr inbounds float, float* %7, i64 %104
  %106 = bitcast float* %105 to i32*
  %107 = load i32, i32* %106, align 4, !tbaa !20
  %108 = getelementptr inbounds float, float* %4, i64 %102
  %109 = bitcast float* %108 to i32*
  store i32 %107, i32* %109, align 4, !tbaa !23
  %110 = or i64 %28, 10
  %111 = add i32 %31, 50
  %112 = sext i32 %111 to i64
  %113 = getelementptr inbounds float, float* %7, i64 %112
  %114 = bitcast float* %113 to i32*
  %115 = load i32, i32* %114, align 4, !tbaa !20
  %116 = getelementptr inbounds float, float* %4, i64 %110
  %117 = bitcast float* %116 to i32*
  store i32 %115, i32* %117, align 4, !tbaa !23
  %118 = or i64 %28, 11
  %119 = add i32 %31, 55
  %120 = sext i32 %119 to i64
  %121 = getelementptr inbounds float, float* %7, i64 %120
  %122 = bitcast float* %121 to i32*
  %123 = load i32, i32* %122, align 4, !tbaa !20
  %124 = getelementptr inbounds float, float* %4, i64 %118
  %125 = bitcast float* %124 to i32*
  store i32 %123, i32* %125, align 4, !tbaa !23
  %126 = or i64 %28, 12
  %127 = add i32 %31, 60
  %128 = sext i32 %127 to i64
  %129 = getelementptr inbounds float, float* %7, i64 %128
  %130 = bitcast float* %129 to i32*
  %131 = load i32, i32* %130, align 4, !tbaa !20
  %132 = getelementptr inbounds float, float* %4, i64 %126
  %133 = bitcast float* %132 to i32*
  store i32 %131, i32* %133, align 4, !tbaa !23
  %134 = or i64 %28, 13
  %135 = add i32 %31, 65
  %136 = sext i32 %135 to i64
  %137 = getelementptr inbounds float, float* %7, i64 %136
  %138 = bitcast float* %137 to i32*
  %139 = load i32, i32* %138, align 4, !tbaa !20
  %140 = getelementptr inbounds float, float* %4, i64 %134
  %141 = bitcast float* %140 to i32*
  store i32 %139, i32* %141, align 4, !tbaa !23
  %142 = or i64 %28, 14
  %143 = add i32 %31, 70
  %144 = sext i32 %143 to i64
  %145 = getelementptr inbounds float, float* %7, i64 %144
  %146 = bitcast float* %145 to i32*
  %147 = load i32, i32* %146, align 4, !tbaa !20
  %148 = getelementptr inbounds float, float* %4, i64 %142
  %149 = bitcast float* %148 to i32*
  store i32 %147, i32* %149, align 4, !tbaa !23
  %150 = or i64 %28, 15
  %151 = add i32 %31, 75
  %152 = sext i32 %151 to i64
  %153 = getelementptr inbounds float, float* %7, i64 %152
  %154 = bitcast float* %153 to i32*
  %155 = load i32, i32* %154, align 4, !tbaa !20
  %156 = getelementptr inbounds float, float* %4, i64 %150
  %157 = bitcast float* %156 to i32*
  store i32 %155, i32* %157, align 4, !tbaa !23
  %158 = or i64 %28, 16
  %159 = add i32 %31, 80
  %160 = sext i32 %159 to i64
  %161 = getelementptr inbounds float, float* %7, i64 %160
  %162 = bitcast float* %161 to i32*
  %163 = load i32, i32* %162, align 4, !tbaa !20
  %164 = getelementptr inbounds float, float* %4, i64 %158
  %165 = bitcast float* %164 to i32*
  store i32 %163, i32* %165, align 4, !tbaa !23
  %166 = or i64 %28, 17
  %167 = add i32 %31, 85
  %168 = sext i32 %167 to i64
  %169 = getelementptr inbounds float, float* %7, i64 %168
  %170 = bitcast float* %169 to i32*
  %171 = load i32, i32* %170, align 4, !tbaa !20
  %172 = getelementptr inbounds float, float* %4, i64 %166
  %173 = bitcast float* %172 to i32*
  store i32 %171, i32* %173, align 4, !tbaa !23
  %174 = or i64 %28, 18
  %175 = add i32 %31, 90
  %176 = sext i32 %175 to i64
  %177 = getelementptr inbounds float, float* %7, i64 %176
  %178 = bitcast float* %177 to i32*
  %179 = load i32, i32* %178, align 4, !tbaa !20
  %180 = getelementptr inbounds float, float* %4, i64 %174
  %181 = bitcast float* %180 to i32*
  store i32 %179, i32* %181, align 4, !tbaa !23
  %182 = or i64 %28, 19
  %183 = add i32 %31, 95
  %184 = sext i32 %183 to i64
  %185 = getelementptr inbounds float, float* %7, i64 %184
  %186 = bitcast float* %185 to i32*
  %187 = load i32, i32* %186, align 4, !tbaa !20
  %188 = getelementptr inbounds float, float* %4, i64 %182
  %189 = bitcast float* %188 to i32*
  store i32 %187, i32* %189, align 4, !tbaa !23
  %190 = or i64 %28, 20
  %191 = add i32 %31, 100
  %192 = sext i32 %191 to i64
  %193 = getelementptr inbounds float, float* %7, i64 %192
  %194 = bitcast float* %193 to i32*
  %195 = load i32, i32* %194, align 4, !tbaa !20
  %196 = getelementptr inbounds float, float* %4, i64 %190
  %197 = bitcast float* %196 to i32*
  store i32 %195, i32* %197, align 4, !tbaa !23
  %198 = or i64 %28, 21
  %199 = add i32 %31, 105
  %200 = sext i32 %199 to i64
  %201 = getelementptr inbounds float, float* %7, i64 %200
  %202 = bitcast float* %201 to i32*
  %203 = load i32, i32* %202, align 4, !tbaa !20
  %204 = getelementptr inbounds float, float* %4, i64 %198
  %205 = bitcast float* %204 to i32*
  store i32 %203, i32* %205, align 4, !tbaa !23
  %206 = or i64 %28, 22
  %207 = add i32 %31, 110
  %208 = sext i32 %207 to i64
  %209 = getelementptr inbounds float, float* %7, i64 %208
  %210 = bitcast float* %209 to i32*
  %211 = load i32, i32* %210, align 4, !tbaa !20
  %212 = getelementptr inbounds float, float* %4, i64 %206
  %213 = bitcast float* %212 to i32*
  store i32 %211, i32* %213, align 4, !tbaa !23
  %214 = or i64 %28, 23
  %215 = add i32 %31, 115
  %216 = sext i32 %215 to i64
  %217 = getelementptr inbounds float, float* %7, i64 %216
  %218 = bitcast float* %217 to i32*
  %219 = load i32, i32* %218, align 4, !tbaa !20
  %220 = getelementptr inbounds float, float* %4, i64 %214
  %221 = bitcast float* %220 to i32*
  store i32 %219, i32* %221, align 4, !tbaa !23
  %222 = or i64 %28, 24
  %223 = add i32 %31, 120
  %224 = sext i32 %223 to i64
  %225 = getelementptr inbounds float, float* %7, i64 %224
  %226 = bitcast float* %225 to i32*
  %227 = load i32, i32* %226, align 4, !tbaa !20
  %228 = getelementptr inbounds float, float* %4, i64 %222
  %229 = bitcast float* %228 to i32*
  store i32 %227, i32* %229, align 4, !tbaa !23
  %230 = or i64 %28, 25
  %231 = add i32 %31, 125
  %232 = sext i32 %231 to i64
  %233 = getelementptr inbounds float, float* %7, i64 %232
  %234 = bitcast float* %233 to i32*
  %235 = load i32, i32* %234, align 4, !tbaa !20
  %236 = getelementptr inbounds float, float* %4, i64 %230
  %237 = bitcast float* %236 to i32*
  store i32 %235, i32* %237, align 4, !tbaa !23
  %238 = or i64 %28, 26
  %239 = add i32 %31, 130
  %240 = sext i32 %239 to i64
  %241 = getelementptr inbounds float, float* %7, i64 %240
  %242 = bitcast float* %241 to i32*
  %243 = load i32, i32* %242, align 4, !tbaa !20
  %244 = getelementptr inbounds float, float* %4, i64 %238
  %245 = bitcast float* %244 to i32*
  store i32 %243, i32* %245, align 4, !tbaa !23
  %246 = or i64 %28, 27
  %247 = add i32 %31, 135
  %248 = sext i32 %247 to i64
  %249 = getelementptr inbounds float, float* %7, i64 %248
  %250 = bitcast float* %249 to i32*
  %251 = load i32, i32* %250, align 4, !tbaa !20
  %252 = getelementptr inbounds float, float* %4, i64 %246
  %253 = bitcast float* %252 to i32*
  store i32 %251, i32* %253, align 4, !tbaa !23
  %254 = or i64 %28, 28
  %255 = add i32 %31, 140
  %256 = sext i32 %255 to i64
  %257 = getelementptr inbounds float, float* %7, i64 %256
  %258 = bitcast float* %257 to i32*
  %259 = load i32, i32* %258, align 4, !tbaa !20
  %260 = getelementptr inbounds float, float* %4, i64 %254
  %261 = bitcast float* %260 to i32*
  store i32 %259, i32* %261, align 4, !tbaa !23
  %262 = or i64 %28, 29
  %263 = add i32 %31, 145
  %264 = sext i32 %263 to i64
  %265 = getelementptr inbounds float, float* %7, i64 %264
  %266 = bitcast float* %265 to i32*
  %267 = load i32, i32* %266, align 4, !tbaa !20
  %268 = getelementptr inbounds float, float* %4, i64 %262
  %269 = bitcast float* %268 to i32*
  store i32 %267, i32* %269, align 4, !tbaa !23
  %270 = or i64 %28, 30
  %271 = add i32 %31, 150
  %272 = sext i32 %271 to i64
  %273 = getelementptr inbounds float, float* %7, i64 %272
  %274 = bitcast float* %273 to i32*
  %275 = load i32, i32* %274, align 4, !tbaa !20
  %276 = getelementptr inbounds float, float* %4, i64 %270
  %277 = bitcast float* %276 to i32*
  store i32 %275, i32* %277, align 4, !tbaa !23
  %278 = or i64 %28, 31
  %279 = add i32 %31, 155
  %280 = sext i32 %279 to i64
  %281 = getelementptr inbounds float, float* %7, i64 %280
  %282 = bitcast float* %281 to i32*
  %283 = load i32, i32* %282, align 4, !tbaa !20
  %284 = getelementptr inbounds float, float* %4, i64 %278
  %285 = bitcast float* %284 to i32*
  store i32 %283, i32* %285, align 4, !tbaa !23
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 32
  br i1 %exitcond, label %for_end3, label %for_body2, !prof !26

for_end3:                                         ; preds = %for_body2
  %indvars.iv.next8 = add nsw i64 %indvars.iv7, 1
  %286 = icmp slt i64 %indvars.iv.next8, %23
  br i1 %286, label %for_body, label %for_end, !prof !19
}

define dllexport i32 @fused_layout_transform_2(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !27 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !29, metadata !DIExpression()), !dbg !32
  call void @llvm.dbg.value(metadata i8* %1, metadata !30, metadata !DIExpression()), !dbg !32
  call void @llvm.dbg.value(metadata i32 %2, metadata !31, metadata !DIExpression()), !dbg !32
  %3 = bitcast i8* %0 to %1**, !dbg !32
  %4 = load %1*, %1** %3, align 8, !dbg !32
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !32
  %6 = bitcast i8* %5 to %1**, !dbg !32
  %7 = load %1*, %1** %6, align 8, !dbg !32
  %8 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !32
  %9 = load i8*, i8** %8, align 8, !dbg !32
  %10 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !32
  %11 = load i8*, i8** %10, align 8, !dbg !32
  %12 = tail call fastcc i32 @fused_layout_transform_2_compute_(i8* %11, i8* %9), !dbg !32
  ret i32 %12, !dbg !32
}

; Function Attrs: noinline
define private fastcc i32 @fused_layout_transform_2_compute_(i8* noalias, i8* noalias) unnamed_addr #0 {
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
  %10 = add nsw i32 %9, 31
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 32
  %15 = select i1 %14, i32 %13, i32 32
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 32
  %18 = select i1 %17, i32 %16, i32 32
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
  %25 = shl i32 %24, 5
  %26 = sext i32 %25 to i64
  %27 = getelementptr inbounds float, float* %7, i64 %26
  %28 = getelementptr inbounds float, float* %4, i64 %26
  %29 = bitcast float* %27 to <4 x i32>*
  %30 = load <4 x i32>, <4 x i32>* %29, align 4, !tbaa !33
  %31 = bitcast float* %28 to <4 x i32>*
  store <4 x i32> %30, <4 x i32>* %31, align 4, !tbaa !36
  %32 = or i64 %26, 4
  %33 = getelementptr inbounds float, float* %7, i64 %32
  %34 = getelementptr inbounds float, float* %4, i64 %32
  %35 = bitcast float* %33 to <4 x i32>*
  %36 = load <4 x i32>, <4 x i32>* %35, align 4, !tbaa !33
  %37 = bitcast float* %34 to <4 x i32>*
  store <4 x i32> %36, <4 x i32>* %37, align 4, !tbaa !36
  %38 = or i64 %26, 8
  %39 = getelementptr inbounds float, float* %7, i64 %38
  %40 = getelementptr inbounds float, float* %4, i64 %38
  %41 = bitcast float* %39 to <4 x i32>*
  %42 = load <4 x i32>, <4 x i32>* %41, align 4, !tbaa !33
  %43 = bitcast float* %40 to <4 x i32>*
  store <4 x i32> %42, <4 x i32>* %43, align 4, !tbaa !36
  %44 = or i64 %26, 12
  %45 = getelementptr inbounds float, float* %7, i64 %44
  %46 = getelementptr inbounds float, float* %4, i64 %44
  %47 = bitcast float* %45 to <4 x i32>*
  %48 = load <4 x i32>, <4 x i32>* %47, align 4, !tbaa !33
  %49 = bitcast float* %46 to <4 x i32>*
  store <4 x i32> %48, <4 x i32>* %49, align 4, !tbaa !36
  %50 = or i64 %26, 16
  %51 = getelementptr inbounds float, float* %7, i64 %50
  %52 = getelementptr inbounds float, float* %4, i64 %50
  %53 = bitcast float* %51 to <4 x i32>*
  %54 = load <4 x i32>, <4 x i32>* %53, align 4, !tbaa !33
  %55 = bitcast float* %52 to <4 x i32>*
  store <4 x i32> %54, <4 x i32>* %55, align 4, !tbaa !36
  %56 = or i64 %26, 20
  %57 = getelementptr inbounds float, float* %7, i64 %56
  %58 = getelementptr inbounds float, float* %4, i64 %56
  %59 = bitcast float* %57 to <4 x i32>*
  %60 = load <4 x i32>, <4 x i32>* %59, align 4, !tbaa !33
  %61 = bitcast float* %58 to <4 x i32>*
  store <4 x i32> %60, <4 x i32>* %61, align 4, !tbaa !36
  %62 = or i64 %26, 24
  %63 = getelementptr inbounds float, float* %7, i64 %62
  %64 = getelementptr inbounds float, float* %4, i64 %62
  %65 = bitcast float* %63 to <4 x i32>*
  %66 = load <4 x i32>, <4 x i32>* %65, align 4, !tbaa !33
  %67 = bitcast float* %64 to <4 x i32>*
  store <4 x i32> %66, <4 x i32>* %67, align 4, !tbaa !36
  %68 = or i64 %26, 28
  %69 = getelementptr inbounds float, float* %7, i64 %68
  %70 = getelementptr inbounds float, float* %4, i64 %68
  %71 = bitcast float* %69 to <4 x i32>*
  %72 = load <4 x i32>, <4 x i32>* %71, align 4, !tbaa !33
  %73 = bitcast float* %70 to <4 x i32>*
  store <4 x i32> %72, <4 x i32>* %73, align 4, !tbaa !36
  %indvars.iv.next = add nsw i64 %indvars.iv, 1
  %74 = icmp slt i64 %indvars.iv.next, %23
  br i1 %74, label %for_body, label %for_end, !prof !19

for_end:                                          ; preds = %for_body, %entry
  ret i32 0
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
  %5 = tail call i8* %4(i32 1, i32 %3, i64 4624, i32 2, i32 32)
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
  %10 = add nsw i32 %9, 33
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 34
  %15 = select i1 %14, i32 %13, i32 34
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 34
  %18 = select i1 %17, i32 %16, i32 34
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %20 = xor i32 %16, -1
  %21 = icmp sgt i32 %20, -35
  %smax = select i1 %21, i32 %20, i32 -35
  %22 = mul i32 %smax, -34
  %23 = add i32 %22, -34
  %24 = add i32 %18, 1
  %25 = sext i32 %24 to i64
  %26 = add nsw i64 %25, -1
  %27 = sext i32 %15 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv6 = phi i64 [ %26, %for_body.lr.ph ], [ %indvars.iv.next7, %for_end3 ]
  %indvar = phi i32 [ 0, %for_body.lr.ph ], [ %indvar.next, %for_end3 ]
  %28 = mul nsw i64 %indvars.iv6, 34
  %29 = trunc i64 %indvars.iv6 to i32
  %.off = add i32 %29, -1
  %30 = icmp ult i32 %.off, 32
  %31 = shl i32 %29, 5
  %32 = add i32 %31, -33
  br i1 %30, label %for_body2.us.preheader, label %for_body.split

for_body2.us.preheader:                           ; preds = %for_body
  br label %for_body2.us

for_body2.us:                                     ; preds = %for_body2.us.preheader, %if_end.us
  %indvars.iv = phi i64 [ %indvars.iv.next, %if_end.us ], [ 0, %for_body2.us.preheader ]
  %33 = phi i32 [ %42, %if_end.us ], [ 0, %for_body2.us.preheader ]
  %34 = add nsw i64 %indvars.iv, %28
  %trunc.us = trunc i32 %33 to i31
  switch i31 %trunc.us, label %if_then.us [
    i31 33, label %if_end.us
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
  %exitcond = icmp eq i64 %indvars.iv.next, 34
  br i1 %exitcond, label %for_end3, label %for_body2.us, !prof !26

for_body.split:                                   ; preds = %for_body
  %43 = mul i32 %indvar, 34
  %44 = add i32 %23, %43
  %45 = sext i32 %44 to i64
  %scevgep = getelementptr float, float* %4, i64 %45
  %scevgep5 = bitcast float* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* %scevgep5, i8 0, i64 136, i32 4, i1 false)
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
  %3 = alloca [16 x <5 x float>], align 16
  %4 = bitcast [16 x <5 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0
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
  %15 = add nsw i32 %14, 31
  %16 = sdiv i32 %15, %14
  %17 = add nsw i32 %0, 1
  %18 = mul nsw i32 %16, %17
  %19 = icmp slt i32 %18, 32
  %20 = select i1 %19, i32 %18, i32 32
  %21 = mul nsw i32 %16, %0
  %22 = icmp slt i32 %21, 32
  %23 = select i1 %22, i32 %21, i32 32
  %24 = icmp slt i32 %23, %20
  br i1 %24, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %25 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 5
  %26 = bitcast float* %25 to <5 x float>*
  %27 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 10
  %28 = bitcast float* %27 to <5 x float>*
  %29 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 15
  %30 = bitcast float* %29 to <5 x float>*
  %31 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 20
  %32 = bitcast float* %31 to <5 x float>*
  %33 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 25
  %34 = bitcast float* %33 to <5 x float>*
  %35 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 30
  %36 = bitcast float* %35 to <5 x float>*
  %37 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 35
  %38 = bitcast float* %37 to <5 x float>*
  %39 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 40
  %40 = bitcast float* %39 to <5 x float>*
  %41 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 45
  %42 = bitcast float* %41 to <5 x float>*
  %43 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 50
  %44 = bitcast float* %43 to <5 x float>*
  %45 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 55
  %46 = bitcast float* %45 to <5 x float>*
  %47 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 60
  %48 = bitcast float* %47 to <5 x float>*
  %49 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 65
  %50 = bitcast float* %49 to <5 x float>*
  %51 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 70
  %52 = bitcast float* %51 to <5 x float>*
  %53 = getelementptr inbounds [16 x <5 x float>], [16 x <5 x float>]* %3, i64 0, i64 0, i64 75
  %54 = bitcast float* %53 to <5 x float>*
  %55 = xor i32 %21, -1
  %56 = icmp sgt i32 %55, -33
  %smax = select i1 %56, i32 %55, i32 -33
  %57 = mul i32 %smax, -160
  %58 = add i32 %57, -160
  %59 = add i32 %23, 1
  %60 = sext i32 %59 to i64
  %61 = add nsw i64 %60, -1
  %62 = sext i32 %20 to i64
  %63 = bitcast [16 x <5 x float>]* %3 to i8*
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv95 = phi i64 [ %61, %for_body.lr.ph ], [ %indvars.iv.next96, %for_end3 ]
  %indvar = phi i32 [ 0, %for_body.lr.ph ], [ %indvar.next, %for_end3 ]
  %64 = mul i32 %indvar, 160
  %65 = add i32 %58, %64
  %66 = trunc i64 %indvars.iv95 to i32
  br label %for_body2

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_body2:                                        ; preds = %for_end6, %for_body
  %indvars.iv92 = phi i64 [ 0, %for_body ], [ %indvars.iv.next93, %for_end6 ]
  %67 = trunc i64 %indvars.iv92 to i32
  %68 = mul i32 %67, 80
  %69 = add i32 %65, %68
  %70 = sext i32 %69 to i64
  %scevgep = getelementptr float, float* %12, i64 %70
  %scevgep91 = bitcast float* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* nonnull %63, i8 0, i64 320, i32 16, i1 false)
  %indvars.iv92.tr = trunc i64 %indvars.iv92 to i32
  %71 = shl i32 %indvars.iv92.tr, 4
  br label %for_body5

for_end3:                                         ; preds = %for_end6
  %indvars.iv.next96 = add nsw i64 %indvars.iv95, 1
  %72 = icmp slt i64 %indvars.iv.next96, %62
  %indvar.next = add nuw i32 %indvar, 1
  br i1 %72, label %for_body, label %for_end, !prof !19

for_body5:                                        ; preds = %for_body5, %for_body2
  %indvars.iv = phi i64 [ 0, %for_body2 ], [ %indvars.iv.next, %for_body5 ]
  %.lcssa4172 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %294, %for_body5 ]
  %.lcssa3970 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %288, %for_body5 ]
  %.lcssa3768 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %287, %for_body5 ]
  %.lcssa3566 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %281, %for_body5 ]
  %.lcssa3364 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %275, %for_body5 ]
  %.lcssa3162 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %269, %for_body5 ]
  %.lcssa2960 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %263, %for_body5 ]
  %.lcssa2758 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %257, %for_body5 ]
  %.lcssa2556 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %251, %for_body5 ]
  %.lcssa2354 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %245, %for_body5 ]
  %.lcssa2152 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %239, %for_body5 ]
  %.lcssa1950 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %233, %for_body5 ]
  %.lcssa1748 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %227, %for_body5 ]
  %.lcssa1546 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %221, %for_body5 ]
  %.lcssa1345 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %215, %for_body5 ]
  %.lcssa43 = phi <5 x float> [ zeroinitializer, %for_body2 ], [ %209, %for_body5 ]
  %73 = trunc i64 %indvars.iv to i32
  %74 = add i32 %73, %66
  %75 = mul i32 %74, 34
  %76 = add nsw i32 %75, %71
  %77 = mul nuw nsw i64 %indvars.iv, 15
  %78 = sext i32 %76 to i64
  %79 = getelementptr inbounds float, float* %6, i64 %78
  %80 = load float, float* %79, align 4, !tbaa !48
  %81 = insertelement <5 x float> undef, float %80, i32 0
  %82 = shufflevector <5 x float> %81, <5 x float> undef, <5 x i32> zeroinitializer
  %83 = getelementptr inbounds float, float* %9, i64 %77
  %84 = bitcast float* %83 to <5 x float>*
  %85 = load <5 x float>, <5 x float>* %84, align 4, !tbaa !51
  %86 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %82, <5 x float> %85, <5 x float> %.lcssa43)
  %87 = or i64 %78, 1
  %88 = getelementptr inbounds float, float* %6, i64 %87
  %89 = load float, float* %88, align 4, !tbaa !48
  %90 = insertelement <5 x float> undef, float %89, i32 0
  %91 = shufflevector <5 x float> %90, <5 x float> undef, <5 x i32> zeroinitializer
  %92 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %91, <5 x float> %85, <5 x float> %.lcssa1345)
  %93 = add nsw i64 %78, 2
  %94 = getelementptr inbounds float, float* %6, i64 %93
  %95 = load float, float* %94, align 4, !tbaa !48
  %96 = insertelement <5 x float> undef, float %95, i32 0
  %97 = shufflevector <5 x float> %96, <5 x float> undef, <5 x i32> zeroinitializer
  %98 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %97, <5 x float> %85, <5 x float> %.lcssa1546)
  %99 = add nsw i64 %78, 3
  %100 = getelementptr inbounds float, float* %6, i64 %99
  %101 = load float, float* %100, align 4, !tbaa !48
  %102 = insertelement <5 x float> undef, float %101, i32 0
  %103 = shufflevector <5 x float> %102, <5 x float> undef, <5 x i32> zeroinitializer
  %104 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %103, <5 x float> %85, <5 x float> %.lcssa1748)
  %105 = add nsw i64 %78, 4
  %106 = getelementptr inbounds float, float* %6, i64 %105
  %107 = load float, float* %106, align 4, !tbaa !48
  %108 = insertelement <5 x float> undef, float %107, i32 0
  %109 = shufflevector <5 x float> %108, <5 x float> undef, <5 x i32> zeroinitializer
  %110 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %109, <5 x float> %85, <5 x float> %.lcssa1950)
  %111 = add nsw i64 %78, 5
  %112 = getelementptr inbounds float, float* %6, i64 %111
  %113 = load float, float* %112, align 4, !tbaa !48
  %114 = insertelement <5 x float> undef, float %113, i32 0
  %115 = shufflevector <5 x float> %114, <5 x float> undef, <5 x i32> zeroinitializer
  %116 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %115, <5 x float> %85, <5 x float> %.lcssa2152)
  %117 = add nsw i64 %78, 6
  %118 = getelementptr inbounds float, float* %6, i64 %117
  %119 = load float, float* %118, align 4, !tbaa !48
  %120 = insertelement <5 x float> undef, float %119, i32 0
  %121 = shufflevector <5 x float> %120, <5 x float> undef, <5 x i32> zeroinitializer
  %122 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %121, <5 x float> %85, <5 x float> %.lcssa2354)
  %123 = add nsw i64 %78, 7
  %124 = getelementptr inbounds float, float* %6, i64 %123
  %125 = load float, float* %124, align 4, !tbaa !48
  %126 = insertelement <5 x float> undef, float %125, i32 0
  %127 = shufflevector <5 x float> %126, <5 x float> undef, <5 x i32> zeroinitializer
  %128 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %127, <5 x float> %85, <5 x float> %.lcssa2556)
  %129 = add nsw i64 %78, 8
  %130 = getelementptr inbounds float, float* %6, i64 %129
  %131 = load float, float* %130, align 4, !tbaa !48
  %132 = insertelement <5 x float> undef, float %131, i32 0
  %133 = shufflevector <5 x float> %132, <5 x float> undef, <5 x i32> zeroinitializer
  %134 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %133, <5 x float> %85, <5 x float> %.lcssa2758)
  %135 = add nsw i64 %78, 9
  %136 = getelementptr inbounds float, float* %6, i64 %135
  %137 = load float, float* %136, align 4, !tbaa !48
  %138 = insertelement <5 x float> undef, float %137, i32 0
  %139 = shufflevector <5 x float> %138, <5 x float> undef, <5 x i32> zeroinitializer
  %140 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %139, <5 x float> %85, <5 x float> %.lcssa2960)
  %141 = add nsw i64 %78, 10
  %142 = getelementptr inbounds float, float* %6, i64 %141
  %143 = load float, float* %142, align 4, !tbaa !48
  %144 = insertelement <5 x float> undef, float %143, i32 0
  %145 = shufflevector <5 x float> %144, <5 x float> undef, <5 x i32> zeroinitializer
  %146 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %145, <5 x float> %85, <5 x float> %.lcssa3162)
  %147 = add nsw i64 %78, 11
  %148 = getelementptr inbounds float, float* %6, i64 %147
  %149 = load float, float* %148, align 4, !tbaa !48
  %150 = insertelement <5 x float> undef, float %149, i32 0
  %151 = shufflevector <5 x float> %150, <5 x float> undef, <5 x i32> zeroinitializer
  %152 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %151, <5 x float> %85, <5 x float> %.lcssa3364)
  %153 = add nsw i64 %78, 12
  %154 = getelementptr inbounds float, float* %6, i64 %153
  %155 = load float, float* %154, align 4, !tbaa !48
  %156 = insertelement <5 x float> undef, float %155, i32 0
  %157 = shufflevector <5 x float> %156, <5 x float> undef, <5 x i32> zeroinitializer
  %158 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %157, <5 x float> %85, <5 x float> %.lcssa3566)
  %159 = add nsw i64 %78, 13
  %160 = getelementptr inbounds float, float* %6, i64 %159
  %161 = load float, float* %160, align 4, !tbaa !48
  %162 = insertelement <5 x float> undef, float %161, i32 0
  %163 = shufflevector <5 x float> %162, <5 x float> undef, <5 x i32> zeroinitializer
  %164 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %163, <5 x float> %85, <5 x float> %.lcssa3768)
  %165 = add nsw i64 %78, 14
  %166 = getelementptr inbounds float, float* %6, i64 %165
  %167 = load float, float* %166, align 4, !tbaa !48
  %168 = insertelement <5 x float> undef, float %167, i32 0
  %169 = shufflevector <5 x float> %168, <5 x float> undef, <5 x i32> zeroinitializer
  %170 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %169, <5 x float> %85, <5 x float> %.lcssa3970)
  %171 = add nsw i64 %78, 15
  %172 = getelementptr inbounds float, float* %6, i64 %171
  %173 = load float, float* %172, align 4, !tbaa !48
  %174 = insertelement <5 x float> undef, float %173, i32 0
  %175 = shufflevector <5 x float> %174, <5 x float> undef, <5 x i32> zeroinitializer
  %176 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %175, <5 x float> %85, <5 x float> %.lcssa4172)
  %177 = add nuw nsw i64 %77, 5
  %178 = getelementptr inbounds float, float* %9, i64 %177
  %179 = bitcast float* %178 to <5 x float>*
  %180 = load <5 x float>, <5 x float>* %179, align 4, !tbaa !51
  %181 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %91, <5 x float> %180, <5 x float> %86)
  %182 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %97, <5 x float> %180, <5 x float> %92)
  %183 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %103, <5 x float> %180, <5 x float> %98)
  %184 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %109, <5 x float> %180, <5 x float> %104)
  %185 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %115, <5 x float> %180, <5 x float> %110)
  %186 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %121, <5 x float> %180, <5 x float> %116)
  %187 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %127, <5 x float> %180, <5 x float> %122)
  %188 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %133, <5 x float> %180, <5 x float> %128)
  %189 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %139, <5 x float> %180, <5 x float> %134)
  %190 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %145, <5 x float> %180, <5 x float> %140)
  %191 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %151, <5 x float> %180, <5 x float> %146)
  %192 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %157, <5 x float> %180, <5 x float> %152)
  %193 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %163, <5 x float> %180, <5 x float> %158)
  %194 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %169, <5 x float> %180, <5 x float> %164)
  %195 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %175, <5 x float> %180, <5 x float> %170)
  %196 = add nsw i64 %87, 15
  %197 = getelementptr inbounds float, float* %6, i64 %196
  %198 = load float, float* %197, align 4, !tbaa !48
  %199 = insertelement <5 x float> undef, float %198, i32 0
  %200 = shufflevector <5 x float> %199, <5 x float> undef, <5 x i32> zeroinitializer
  %201 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %200, <5 x float> %180, <5 x float> %176)
  %202 = load float, float* %94, align 4, !tbaa !48
  %203 = insertelement <5 x float> undef, float %202, i32 0
  %204 = shufflevector <5 x float> %203, <5 x float> undef, <5 x i32> zeroinitializer
  %205 = add nuw nsw i64 %77, 10
  %206 = getelementptr inbounds float, float* %9, i64 %205
  %207 = bitcast float* %206 to <5 x float>*
  %208 = load <5 x float>, <5 x float>* %207, align 4, !tbaa !51
  %209 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %204, <5 x float> %208, <5 x float> %181)
  %210 = add nsw i64 %78, 3
  %211 = getelementptr inbounds float, float* %6, i64 %210
  %212 = load float, float* %211, align 4, !tbaa !48
  %213 = insertelement <5 x float> undef, float %212, i32 0
  %214 = shufflevector <5 x float> %213, <5 x float> undef, <5 x i32> zeroinitializer
  %215 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %214, <5 x float> %208, <5 x float> %182)
  %216 = add nsw i64 %78, 4
  %217 = getelementptr inbounds float, float* %6, i64 %216
  %218 = load float, float* %217, align 4, !tbaa !48
  %219 = insertelement <5 x float> undef, float %218, i32 0
  %220 = shufflevector <5 x float> %219, <5 x float> undef, <5 x i32> zeroinitializer
  %221 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %220, <5 x float> %208, <5 x float> %183)
  %222 = add nsw i64 %78, 5
  %223 = getelementptr inbounds float, float* %6, i64 %222
  %224 = load float, float* %223, align 4, !tbaa !48
  %225 = insertelement <5 x float> undef, float %224, i32 0
  %226 = shufflevector <5 x float> %225, <5 x float> undef, <5 x i32> zeroinitializer
  %227 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %226, <5 x float> %208, <5 x float> %184)
  %228 = add nsw i64 %78, 6
  %229 = getelementptr inbounds float, float* %6, i64 %228
  %230 = load float, float* %229, align 4, !tbaa !48
  %231 = insertelement <5 x float> undef, float %230, i32 0
  %232 = shufflevector <5 x float> %231, <5 x float> undef, <5 x i32> zeroinitializer
  %233 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %232, <5 x float> %208, <5 x float> %185)
  %234 = add nsw i64 %78, 7
  %235 = getelementptr inbounds float, float* %6, i64 %234
  %236 = load float, float* %235, align 4, !tbaa !48
  %237 = insertelement <5 x float> undef, float %236, i32 0
  %238 = shufflevector <5 x float> %237, <5 x float> undef, <5 x i32> zeroinitializer
  %239 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %238, <5 x float> %208, <5 x float> %186)
  %240 = add nsw i64 %78, 8
  %241 = getelementptr inbounds float, float* %6, i64 %240
  %242 = load float, float* %241, align 4, !tbaa !48
  %243 = insertelement <5 x float> undef, float %242, i32 0
  %244 = shufflevector <5 x float> %243, <5 x float> undef, <5 x i32> zeroinitializer
  %245 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %244, <5 x float> %208, <5 x float> %187)
  %246 = add nsw i64 %78, 9
  %247 = getelementptr inbounds float, float* %6, i64 %246
  %248 = load float, float* %247, align 4, !tbaa !48
  %249 = insertelement <5 x float> undef, float %248, i32 0
  %250 = shufflevector <5 x float> %249, <5 x float> undef, <5 x i32> zeroinitializer
  %251 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %250, <5 x float> %208, <5 x float> %188)
  %252 = add nsw i64 %78, 10
  %253 = getelementptr inbounds float, float* %6, i64 %252
  %254 = load float, float* %253, align 4, !tbaa !48
  %255 = insertelement <5 x float> undef, float %254, i32 0
  %256 = shufflevector <5 x float> %255, <5 x float> undef, <5 x i32> zeroinitializer
  %257 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %256, <5 x float> %208, <5 x float> %189)
  %258 = add nsw i64 %78, 11
  %259 = getelementptr inbounds float, float* %6, i64 %258
  %260 = load float, float* %259, align 4, !tbaa !48
  %261 = insertelement <5 x float> undef, float %260, i32 0
  %262 = shufflevector <5 x float> %261, <5 x float> undef, <5 x i32> zeroinitializer
  %263 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %262, <5 x float> %208, <5 x float> %190)
  %264 = add nsw i64 %78, 12
  %265 = getelementptr inbounds float, float* %6, i64 %264
  %266 = load float, float* %265, align 4, !tbaa !48
  %267 = insertelement <5 x float> undef, float %266, i32 0
  %268 = shufflevector <5 x float> %267, <5 x float> undef, <5 x i32> zeroinitializer
  %269 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %268, <5 x float> %208, <5 x float> %191)
  %270 = add nsw i64 %78, 13
  %271 = getelementptr inbounds float, float* %6, i64 %270
  %272 = load float, float* %271, align 4, !tbaa !48
  %273 = insertelement <5 x float> undef, float %272, i32 0
  %274 = shufflevector <5 x float> %273, <5 x float> undef, <5 x i32> zeroinitializer
  %275 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %274, <5 x float> %208, <5 x float> %192)
  %276 = add nsw i64 %78, 14
  %277 = getelementptr inbounds float, float* %6, i64 %276
  %278 = load float, float* %277, align 4, !tbaa !48
  %279 = insertelement <5 x float> undef, float %278, i32 0
  %280 = shufflevector <5 x float> %279, <5 x float> undef, <5 x i32> zeroinitializer
  %281 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %280, <5 x float> %208, <5 x float> %193)
  %282 = add nsw i64 %78, 15
  %283 = getelementptr inbounds float, float* %6, i64 %282
  %284 = load float, float* %283, align 4, !tbaa !48
  %285 = insertelement <5 x float> undef, float %284, i32 0
  %286 = shufflevector <5 x float> %285, <5 x float> undef, <5 x i32> zeroinitializer
  %287 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %286, <5 x float> %208, <5 x float> %194)
  %288 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %200, <5 x float> %208, <5 x float> %195)
  %289 = add nsw i64 %78, 17
  %290 = getelementptr inbounds float, float* %6, i64 %289
  %291 = load float, float* %290, align 4, !tbaa !48
  %292 = insertelement <5 x float> undef, float %291, i32 0
  %293 = shufflevector <5 x float> %292, <5 x float> undef, <5 x i32> zeroinitializer
  %294 = tail call <5 x float> @llvm.fmuladd.v5f32(<5 x float> %293, <5 x float> %208, <5 x float> %201)
  %indvars.iv.next = add nuw nsw i64 %indvars.iv, 1
  %exitcond = icmp eq i64 %indvars.iv.next, 3
  br i1 %exitcond, label %for_end6, label %for_body5, !prof !26

for_end6:                                         ; preds = %for_body5
  store <5 x float> %209, <5 x float>* %.sub, align 16, !tbaa !54
  store <5 x float> %215, <5 x float>* %26, align 4, !tbaa !65
  store <5 x float> %221, <5 x float>* %28, align 8, !tbaa !66
  store <5 x float> %227, <5 x float>* %30, align 4, !tbaa !66
  store <5 x float> %233, <5 x float>* %32, align 16, !tbaa !65
  store <5 x float> %239, <5 x float>* %34, align 4, !tbaa !67
  store <5 x float> %245, <5 x float>* %36, align 8, !tbaa !67
  store <5 x float> %251, <5 x float>* %38, align 4, !tbaa !65
  store <5 x float> %257, <5 x float>* %40, align 16, !tbaa !54
  store <5 x float> %263, <5 x float>* %42, align 4, !tbaa !66
  store <5 x float> %269, <5 x float>* %44, align 8, !tbaa !65
  store <5 x float> %275, <5 x float>* %46, align 4, !tbaa !65
  store <5 x float> %281, <5 x float>* %48, align 16, !tbaa !68
  store <5 x float> %287, <5 x float>* %50, align 4, !tbaa !65
  store <5 x float> %288, <5 x float>* %52, align 8, !tbaa !65
  store <5 x float> %294, <5 x float>* %54, align 4, !tbaa !66
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep91, i8* nonnull %4, i64 320, i32 4, i1 false)
  %indvars.iv.next93 = add nuw nsw i64 %indvars.iv92, 1
  %exitcond94 = icmp eq i64 %indvars.iv.next93, 2
  br i1 %exitcond94, label %for_end3, label %for_body2, !prof !26
}

; Function Attrs: nounwind readnone speculatable
declare <5 x float> @llvm.fmuladd.v5f32(<5 x float>, <5 x float>, <5 x float>) #3

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
!5 = distinct !DISubprogram(name: "fused_layout_transform_1", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !11)
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
!22 = !{!"0x55e1389e9580", !18, i64 0}
!23 = !{!24, !24, i64 0}
!24 = !{!"float32", !25, i64 0}
!25 = !{!"0x55e138aa89a0", !18, i64 0}
!26 = !{!"branch_weights", i32 1, i32 1048576}
!27 = distinct !DISubprogram(name: "fused_layout_transform_2", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !28)
!28 = !{!29, !30, !31}
!29 = !DILocalVariable(name: "arg1", arg: 1, scope: !27, file: !1, type: !9)
!30 = !DILocalVariable(name: "arg2", arg: 2, scope: !27, file: !1, type: !9)
!31 = !DILocalVariable(name: "arg3", arg: 3, scope: !27, file: !1, type: !8)
!32 = !DILocation(line: 0, scope: !27)
!33 = !{!34, !34, i64 0}
!34 = !{!"float32", !35, i64 0}
!35 = !{!"0x55e138aa5d60", !18, i64 0}
!36 = !{!37, !37, i64 0}
!37 = !{!"float32", !38, i64 0}
!38 = !{!"0x55e138aa5db0", !18, i64 0}
!39 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !40)
!40 = !{!41, !42, !43}
!41 = !DILocalVariable(name: "arg1", arg: 1, scope: !39, file: !1, type: !9)
!42 = !DILocalVariable(name: "arg2", arg: 2, scope: !39, file: !1, type: !9)
!43 = !DILocalVariable(name: "arg3", arg: 3, scope: !39, file: !1, type: !8)
!44 = !DILocation(line: 0, scope: !39)
!45 = !{!46, !46, i64 0}
!46 = !{!"float32", !47, i64 0}
!47 = !{!"0x55e1389cb130", !18, i64 0}
!48 = !{!49, !49, i64 0}
!49 = !{!"float32", !50, i64 0}
!50 = !{!"0x55e138b24080", !18, i64 0}
!51 = !{!52, !52, i64 0}
!52 = !{!"float32", !53, i64 0}
!53 = !{!"0x55e1389f0820", !18, i64 0}
!54 = !{!55, !55, i64 0}
!55 = !{!"0x55e1389f7260.w8.b0", !56, i64 0}
!56 = !{!"0x55e1389f7260.w16.b0", !57, i64 0}
!57 = !{!"0x55e1389f7260.w32.b0", !58, i64 0}
!58 = !{!"0x55e1389f7260.w64.b0", !59, i64 0}
!59 = !{!"0x55e1389f7260.w128.b0", !60, i64 0}
!60 = !{!"0x55e1389f7260.w256.b0", !61, i64 0}
!61 = !{!"0x55e1389f7260.w512.b0", !62, i64 0}
!62 = !{!"0x55e1389f7260.w1024.b0", !63, i64 0}
!63 = !{!"float32", !64, i64 0}
!64 = !{!"0x55e1389f7260", !18, i64 0}
!65 = !{!56, !56, i64 0}
!66 = !{!57, !57, i64 0}
!67 = !{!58, !58, i64 0}
!68 = !{!59, !59, i64 0}
