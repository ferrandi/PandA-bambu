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
%6 = type { float*, i8* }
%7 = type { float*, i8*, i8* }

@__TVMBackendParallelLaunch = linkonce dllexport local_unnamed_addr global i32 (i32 (i32, %0*, i8*)*, i8*, i32)* null, align 8
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
  %10 = add nsw i32 %9, 7
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 8
  %15 = select i1 %14, i32 %13, i32 8
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 8
  %18 = select i1 %17, i32 %16, i32 8
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
  %25 = shl i32 %24, 3
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
  %indvars.iv.next = add nsw i64 %indvars.iv, 1
  %38 = icmp slt i64 %indvars.iv.next, %23
  br i1 %38, label %for_body, label %for_end, !prof !19

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

for_body:                                         ; preds = %for_body.lr.ph, %for_body
  %indvars.iv = phi i64 [ %22, %for_body.lr.ph ], [ %indvars.iv.next, %for_body ]
  %24 = trunc i64 %indvars.iv to i32
  %25 = shl i32 %24, 6
  %26 = sext i32 %25 to i64
  %27 = getelementptr inbounds float, float* %7, i64 %indvars.iv
  %28 = bitcast float* %27 to i32*
  %29 = load i32, i32* %28, align 4, !tbaa !32
  %30 = getelementptr inbounds float, float* %4, i64 %26
  %31 = bitcast float* %30 to i32*
  store i32 %29, i32* %31, align 4, !tbaa !35
  %32 = or i64 %26, 1
  %33 = shl i64 %indvars.iv, 32
  %sext = add i64 %33, 8589934592
  %34 = ashr exact i64 %sext, 32
  %35 = getelementptr inbounds float, float* %7, i64 %34
  %36 = bitcast float* %35 to i32*
  %37 = load i32, i32* %36, align 4, !tbaa !32
  %38 = getelementptr inbounds float, float* %4, i64 %32
  %39 = bitcast float* %38 to i32*
  store i32 %37, i32* %39, align 4, !tbaa !35
  %40 = or i64 %26, 2
  %41 = shl i64 %indvars.iv, 32
  %sext7 = add i64 %41, 17179869184
  %42 = ashr exact i64 %sext7, 32
  %43 = getelementptr inbounds float, float* %7, i64 %42
  %44 = bitcast float* %43 to i32*
  %45 = load i32, i32* %44, align 4, !tbaa !32
  %46 = getelementptr inbounds float, float* %4, i64 %40
  %47 = bitcast float* %46 to i32*
  store i32 %45, i32* %47, align 4, !tbaa !35
  %48 = or i64 %26, 3
  %49 = shl i64 %indvars.iv, 32
  %sext8 = add i64 %49, 25769803776
  %50 = ashr exact i64 %sext8, 32
  %51 = getelementptr inbounds float, float* %7, i64 %50
  %52 = bitcast float* %51 to i32*
  %53 = load i32, i32* %52, align 4, !tbaa !32
  %54 = getelementptr inbounds float, float* %4, i64 %48
  %55 = bitcast float* %54 to i32*
  store i32 %53, i32* %55, align 4, !tbaa !35
  %56 = or i64 %26, 4
  %57 = shl i64 %indvars.iv, 32
  %sext9 = add i64 %57, 34359738368
  %58 = ashr exact i64 %sext9, 32
  %59 = getelementptr inbounds float, float* %7, i64 %58
  %60 = bitcast float* %59 to i32*
  %61 = load i32, i32* %60, align 4, !tbaa !32
  %62 = getelementptr inbounds float, float* %4, i64 %56
  %63 = bitcast float* %62 to i32*
  store i32 %61, i32* %63, align 4, !tbaa !35
  %64 = or i64 %26, 5
  %65 = shl i64 %indvars.iv, 32
  %sext10 = add i64 %65, 42949672960
  %66 = ashr exact i64 %sext10, 32
  %67 = getelementptr inbounds float, float* %7, i64 %66
  %68 = bitcast float* %67 to i32*
  %69 = load i32, i32* %68, align 4, !tbaa !32
  %70 = getelementptr inbounds float, float* %4, i64 %64
  %71 = bitcast float* %70 to i32*
  store i32 %69, i32* %71, align 4, !tbaa !35
  %72 = or i64 %26, 6
  %73 = shl i64 %indvars.iv, 32
  %sext11 = add i64 %73, 51539607552
  %74 = ashr exact i64 %sext11, 32
  %75 = getelementptr inbounds float, float* %7, i64 %74
  %76 = bitcast float* %75 to i32*
  %77 = load i32, i32* %76, align 4, !tbaa !32
  %78 = getelementptr inbounds float, float* %4, i64 %72
  %79 = bitcast float* %78 to i32*
  store i32 %77, i32* %79, align 4, !tbaa !35
  %80 = or i64 %26, 7
  %81 = shl i64 %indvars.iv, 32
  %sext12 = add i64 %81, 60129542144
  %82 = ashr exact i64 %sext12, 32
  %83 = getelementptr inbounds float, float* %7, i64 %82
  %84 = bitcast float* %83 to i32*
  %85 = load i32, i32* %84, align 4, !tbaa !32
  %86 = getelementptr inbounds float, float* %4, i64 %80
  %87 = bitcast float* %86 to i32*
  store i32 %85, i32* %87, align 4, !tbaa !35
  %88 = or i64 %26, 8
  %89 = shl i64 %indvars.iv, 32
  %sext62 = add i64 %89, 68719476736
  %90 = ashr exact i64 %sext62, 32
  %91 = getelementptr inbounds float, float* %7, i64 %90
  %92 = bitcast float* %91 to i32*
  %93 = load i32, i32* %92, align 4, !tbaa !32
  %94 = getelementptr inbounds float, float* %4, i64 %88
  %95 = bitcast float* %94 to i32*
  store i32 %93, i32* %95, align 4, !tbaa !35
  %96 = or i64 %26, 9
  %97 = shl i64 %indvars.iv, 32
  %sext13 = add i64 %97, 77309411328
  %98 = ashr exact i64 %sext13, 32
  %99 = getelementptr inbounds float, float* %7, i64 %98
  %100 = bitcast float* %99 to i32*
  %101 = load i32, i32* %100, align 4, !tbaa !32
  %102 = getelementptr inbounds float, float* %4, i64 %96
  %103 = bitcast float* %102 to i32*
  store i32 %101, i32* %103, align 4, !tbaa !35
  %104 = or i64 %26, 10
  %105 = shl i64 %indvars.iv, 32
  %sext14 = add i64 %105, 85899345920
  %106 = ashr exact i64 %sext14, 32
  %107 = getelementptr inbounds float, float* %7, i64 %106
  %108 = bitcast float* %107 to i32*
  %109 = load i32, i32* %108, align 4, !tbaa !32
  %110 = getelementptr inbounds float, float* %4, i64 %104
  %111 = bitcast float* %110 to i32*
  store i32 %109, i32* %111, align 4, !tbaa !35
  %112 = or i64 %26, 11
  %113 = shl i64 %indvars.iv, 32
  %sext15 = add i64 %113, 94489280512
  %114 = ashr exact i64 %sext15, 32
  %115 = getelementptr inbounds float, float* %7, i64 %114
  %116 = bitcast float* %115 to i32*
  %117 = load i32, i32* %116, align 4, !tbaa !32
  %118 = getelementptr inbounds float, float* %4, i64 %112
  %119 = bitcast float* %118 to i32*
  store i32 %117, i32* %119, align 4, !tbaa !35
  %120 = or i64 %26, 12
  %121 = shl i64 %indvars.iv, 32
  %sext16 = add i64 %121, 103079215104
  %122 = ashr exact i64 %sext16, 32
  %123 = getelementptr inbounds float, float* %7, i64 %122
  %124 = bitcast float* %123 to i32*
  %125 = load i32, i32* %124, align 4, !tbaa !32
  %126 = getelementptr inbounds float, float* %4, i64 %120
  %127 = bitcast float* %126 to i32*
  store i32 %125, i32* %127, align 4, !tbaa !35
  %128 = or i64 %26, 13
  %129 = shl i64 %indvars.iv, 32
  %sext17 = add i64 %129, 111669149696
  %130 = ashr exact i64 %sext17, 32
  %131 = getelementptr inbounds float, float* %7, i64 %130
  %132 = bitcast float* %131 to i32*
  %133 = load i32, i32* %132, align 4, !tbaa !32
  %134 = getelementptr inbounds float, float* %4, i64 %128
  %135 = bitcast float* %134 to i32*
  store i32 %133, i32* %135, align 4, !tbaa !35
  %136 = or i64 %26, 14
  %137 = shl i64 %indvars.iv, 32
  %sext18 = add i64 %137, 120259084288
  %138 = ashr exact i64 %sext18, 32
  %139 = getelementptr inbounds float, float* %7, i64 %138
  %140 = bitcast float* %139 to i32*
  %141 = load i32, i32* %140, align 4, !tbaa !32
  %142 = getelementptr inbounds float, float* %4, i64 %136
  %143 = bitcast float* %142 to i32*
  store i32 %141, i32* %143, align 4, !tbaa !35
  %144 = or i64 %26, 15
  %145 = shl i64 %indvars.iv, 32
  %sext19 = add i64 %145, 128849018880
  %146 = ashr exact i64 %sext19, 32
  %147 = getelementptr inbounds float, float* %7, i64 %146
  %148 = bitcast float* %147 to i32*
  %149 = load i32, i32* %148, align 4, !tbaa !32
  %150 = getelementptr inbounds float, float* %4, i64 %144
  %151 = bitcast float* %150 to i32*
  store i32 %149, i32* %151, align 4, !tbaa !35
  %152 = or i64 %26, 16
  %153 = shl i64 %indvars.iv, 32
  %sext63 = add i64 %153, 137438953472
  %154 = ashr exact i64 %sext63, 32
  %155 = getelementptr inbounds float, float* %7, i64 %154
  %156 = bitcast float* %155 to i32*
  %157 = load i32, i32* %156, align 4, !tbaa !32
  %158 = getelementptr inbounds float, float* %4, i64 %152
  %159 = bitcast float* %158 to i32*
  store i32 %157, i32* %159, align 4, !tbaa !35
  %160 = or i64 %26, 17
  %161 = shl i64 %indvars.iv, 32
  %sext20 = add i64 %161, 146028888064
  %162 = ashr exact i64 %sext20, 32
  %163 = getelementptr inbounds float, float* %7, i64 %162
  %164 = bitcast float* %163 to i32*
  %165 = load i32, i32* %164, align 4, !tbaa !32
  %166 = getelementptr inbounds float, float* %4, i64 %160
  %167 = bitcast float* %166 to i32*
  store i32 %165, i32* %167, align 4, !tbaa !35
  %168 = or i64 %26, 18
  %169 = shl i64 %indvars.iv, 32
  %sext21 = add i64 %169, 154618822656
  %170 = ashr exact i64 %sext21, 32
  %171 = getelementptr inbounds float, float* %7, i64 %170
  %172 = bitcast float* %171 to i32*
  %173 = load i32, i32* %172, align 4, !tbaa !32
  %174 = getelementptr inbounds float, float* %4, i64 %168
  %175 = bitcast float* %174 to i32*
  store i32 %173, i32* %175, align 4, !tbaa !35
  %176 = or i64 %26, 19
  %177 = shl i64 %indvars.iv, 32
  %sext22 = add i64 %177, 163208757248
  %178 = ashr exact i64 %sext22, 32
  %179 = getelementptr inbounds float, float* %7, i64 %178
  %180 = bitcast float* %179 to i32*
  %181 = load i32, i32* %180, align 4, !tbaa !32
  %182 = getelementptr inbounds float, float* %4, i64 %176
  %183 = bitcast float* %182 to i32*
  store i32 %181, i32* %183, align 4, !tbaa !35
  %184 = or i64 %26, 20
  %185 = shl i64 %indvars.iv, 32
  %sext23 = add i64 %185, 171798691840
  %186 = ashr exact i64 %sext23, 32
  %187 = getelementptr inbounds float, float* %7, i64 %186
  %188 = bitcast float* %187 to i32*
  %189 = load i32, i32* %188, align 4, !tbaa !32
  %190 = getelementptr inbounds float, float* %4, i64 %184
  %191 = bitcast float* %190 to i32*
  store i32 %189, i32* %191, align 4, !tbaa !35
  %192 = or i64 %26, 21
  %193 = shl i64 %indvars.iv, 32
  %sext24 = add i64 %193, 180388626432
  %194 = ashr exact i64 %sext24, 32
  %195 = getelementptr inbounds float, float* %7, i64 %194
  %196 = bitcast float* %195 to i32*
  %197 = load i32, i32* %196, align 4, !tbaa !32
  %198 = getelementptr inbounds float, float* %4, i64 %192
  %199 = bitcast float* %198 to i32*
  store i32 %197, i32* %199, align 4, !tbaa !35
  %200 = or i64 %26, 22
  %201 = shl i64 %indvars.iv, 32
  %sext25 = add i64 %201, 188978561024
  %202 = ashr exact i64 %sext25, 32
  %203 = getelementptr inbounds float, float* %7, i64 %202
  %204 = bitcast float* %203 to i32*
  %205 = load i32, i32* %204, align 4, !tbaa !32
  %206 = getelementptr inbounds float, float* %4, i64 %200
  %207 = bitcast float* %206 to i32*
  store i32 %205, i32* %207, align 4, !tbaa !35
  %208 = or i64 %26, 23
  %209 = shl i64 %indvars.iv, 32
  %sext26 = add i64 %209, 197568495616
  %210 = ashr exact i64 %sext26, 32
  %211 = getelementptr inbounds float, float* %7, i64 %210
  %212 = bitcast float* %211 to i32*
  %213 = load i32, i32* %212, align 4, !tbaa !32
  %214 = getelementptr inbounds float, float* %4, i64 %208
  %215 = bitcast float* %214 to i32*
  store i32 %213, i32* %215, align 4, !tbaa !35
  %216 = or i64 %26, 24
  %217 = shl i64 %indvars.iv, 32
  %sext64 = add i64 %217, 206158430208
  %218 = ashr exact i64 %sext64, 32
  %219 = getelementptr inbounds float, float* %7, i64 %218
  %220 = bitcast float* %219 to i32*
  %221 = load i32, i32* %220, align 4, !tbaa !32
  %222 = getelementptr inbounds float, float* %4, i64 %216
  %223 = bitcast float* %222 to i32*
  store i32 %221, i32* %223, align 4, !tbaa !35
  %224 = or i64 %26, 25
  %225 = shl i64 %indvars.iv, 32
  %sext27 = add i64 %225, 214748364800
  %226 = ashr exact i64 %sext27, 32
  %227 = getelementptr inbounds float, float* %7, i64 %226
  %228 = bitcast float* %227 to i32*
  %229 = load i32, i32* %228, align 4, !tbaa !32
  %230 = getelementptr inbounds float, float* %4, i64 %224
  %231 = bitcast float* %230 to i32*
  store i32 %229, i32* %231, align 4, !tbaa !35
  %232 = or i64 %26, 26
  %233 = shl i64 %indvars.iv, 32
  %sext28 = add i64 %233, 223338299392
  %234 = ashr exact i64 %sext28, 32
  %235 = getelementptr inbounds float, float* %7, i64 %234
  %236 = bitcast float* %235 to i32*
  %237 = load i32, i32* %236, align 4, !tbaa !32
  %238 = getelementptr inbounds float, float* %4, i64 %232
  %239 = bitcast float* %238 to i32*
  store i32 %237, i32* %239, align 4, !tbaa !35
  %240 = or i64 %26, 27
  %241 = shl i64 %indvars.iv, 32
  %sext29 = add i64 %241, 231928233984
  %242 = ashr exact i64 %sext29, 32
  %243 = getelementptr inbounds float, float* %7, i64 %242
  %244 = bitcast float* %243 to i32*
  %245 = load i32, i32* %244, align 4, !tbaa !32
  %246 = getelementptr inbounds float, float* %4, i64 %240
  %247 = bitcast float* %246 to i32*
  store i32 %245, i32* %247, align 4, !tbaa !35
  %248 = or i64 %26, 28
  %249 = shl i64 %indvars.iv, 32
  %sext30 = add i64 %249, 240518168576
  %250 = ashr exact i64 %sext30, 32
  %251 = getelementptr inbounds float, float* %7, i64 %250
  %252 = bitcast float* %251 to i32*
  %253 = load i32, i32* %252, align 4, !tbaa !32
  %254 = getelementptr inbounds float, float* %4, i64 %248
  %255 = bitcast float* %254 to i32*
  store i32 %253, i32* %255, align 4, !tbaa !35
  %256 = or i64 %26, 29
  %257 = shl i64 %indvars.iv, 32
  %sext31 = add i64 %257, 249108103168
  %258 = ashr exact i64 %sext31, 32
  %259 = getelementptr inbounds float, float* %7, i64 %258
  %260 = bitcast float* %259 to i32*
  %261 = load i32, i32* %260, align 4, !tbaa !32
  %262 = getelementptr inbounds float, float* %4, i64 %256
  %263 = bitcast float* %262 to i32*
  store i32 %261, i32* %263, align 4, !tbaa !35
  %264 = or i64 %26, 30
  %265 = shl i64 %indvars.iv, 32
  %sext32 = add i64 %265, 257698037760
  %266 = ashr exact i64 %sext32, 32
  %267 = getelementptr inbounds float, float* %7, i64 %266
  %268 = bitcast float* %267 to i32*
  %269 = load i32, i32* %268, align 4, !tbaa !32
  %270 = getelementptr inbounds float, float* %4, i64 %264
  %271 = bitcast float* %270 to i32*
  store i32 %269, i32* %271, align 4, !tbaa !35
  %272 = or i64 %26, 31
  %273 = shl i64 %indvars.iv, 32
  %sext33 = add i64 %273, 266287972352
  %274 = ashr exact i64 %sext33, 32
  %275 = getelementptr inbounds float, float* %7, i64 %274
  %276 = bitcast float* %275 to i32*
  %277 = load i32, i32* %276, align 4, !tbaa !32
  %278 = getelementptr inbounds float, float* %4, i64 %272
  %279 = bitcast float* %278 to i32*
  store i32 %277, i32* %279, align 4, !tbaa !35
  %280 = or i64 %26, 32
  %281 = shl i64 %indvars.iv, 32
  %sext65 = add i64 %281, 274877906944
  %282 = ashr exact i64 %sext65, 32
  %283 = getelementptr inbounds float, float* %7, i64 %282
  %284 = bitcast float* %283 to i32*
  %285 = load i32, i32* %284, align 4, !tbaa !32
  %286 = getelementptr inbounds float, float* %4, i64 %280
  %287 = bitcast float* %286 to i32*
  store i32 %285, i32* %287, align 4, !tbaa !35
  %288 = or i64 %26, 33
  %289 = shl i64 %indvars.iv, 32
  %sext34 = add i64 %289, 283467841536
  %290 = ashr exact i64 %sext34, 32
  %291 = getelementptr inbounds float, float* %7, i64 %290
  %292 = bitcast float* %291 to i32*
  %293 = load i32, i32* %292, align 4, !tbaa !32
  %294 = getelementptr inbounds float, float* %4, i64 %288
  %295 = bitcast float* %294 to i32*
  store i32 %293, i32* %295, align 4, !tbaa !35
  %296 = or i64 %26, 34
  %297 = shl i64 %indvars.iv, 32
  %sext35 = add i64 %297, 292057776128
  %298 = ashr exact i64 %sext35, 32
  %299 = getelementptr inbounds float, float* %7, i64 %298
  %300 = bitcast float* %299 to i32*
  %301 = load i32, i32* %300, align 4, !tbaa !32
  %302 = getelementptr inbounds float, float* %4, i64 %296
  %303 = bitcast float* %302 to i32*
  store i32 %301, i32* %303, align 4, !tbaa !35
  %304 = or i64 %26, 35
  %305 = shl i64 %indvars.iv, 32
  %sext36 = add i64 %305, 300647710720
  %306 = ashr exact i64 %sext36, 32
  %307 = getelementptr inbounds float, float* %7, i64 %306
  %308 = bitcast float* %307 to i32*
  %309 = load i32, i32* %308, align 4, !tbaa !32
  %310 = getelementptr inbounds float, float* %4, i64 %304
  %311 = bitcast float* %310 to i32*
  store i32 %309, i32* %311, align 4, !tbaa !35
  %312 = or i64 %26, 36
  %313 = shl i64 %indvars.iv, 32
  %sext37 = add i64 %313, 309237645312
  %314 = ashr exact i64 %sext37, 32
  %315 = getelementptr inbounds float, float* %7, i64 %314
  %316 = bitcast float* %315 to i32*
  %317 = load i32, i32* %316, align 4, !tbaa !32
  %318 = getelementptr inbounds float, float* %4, i64 %312
  %319 = bitcast float* %318 to i32*
  store i32 %317, i32* %319, align 4, !tbaa !35
  %320 = or i64 %26, 37
  %321 = shl i64 %indvars.iv, 32
  %sext38 = add i64 %321, 317827579904
  %322 = ashr exact i64 %sext38, 32
  %323 = getelementptr inbounds float, float* %7, i64 %322
  %324 = bitcast float* %323 to i32*
  %325 = load i32, i32* %324, align 4, !tbaa !32
  %326 = getelementptr inbounds float, float* %4, i64 %320
  %327 = bitcast float* %326 to i32*
  store i32 %325, i32* %327, align 4, !tbaa !35
  %328 = or i64 %26, 38
  %329 = shl i64 %indvars.iv, 32
  %sext39 = add i64 %329, 326417514496
  %330 = ashr exact i64 %sext39, 32
  %331 = getelementptr inbounds float, float* %7, i64 %330
  %332 = bitcast float* %331 to i32*
  %333 = load i32, i32* %332, align 4, !tbaa !32
  %334 = getelementptr inbounds float, float* %4, i64 %328
  %335 = bitcast float* %334 to i32*
  store i32 %333, i32* %335, align 4, !tbaa !35
  %336 = or i64 %26, 39
  %337 = shl i64 %indvars.iv, 32
  %sext40 = add i64 %337, 335007449088
  %338 = ashr exact i64 %sext40, 32
  %339 = getelementptr inbounds float, float* %7, i64 %338
  %340 = bitcast float* %339 to i32*
  %341 = load i32, i32* %340, align 4, !tbaa !32
  %342 = getelementptr inbounds float, float* %4, i64 %336
  %343 = bitcast float* %342 to i32*
  store i32 %341, i32* %343, align 4, !tbaa !35
  %344 = or i64 %26, 40
  %345 = shl i64 %indvars.iv, 32
  %sext66 = add i64 %345, 343597383680
  %346 = ashr exact i64 %sext66, 32
  %347 = getelementptr inbounds float, float* %7, i64 %346
  %348 = bitcast float* %347 to i32*
  %349 = load i32, i32* %348, align 4, !tbaa !32
  %350 = getelementptr inbounds float, float* %4, i64 %344
  %351 = bitcast float* %350 to i32*
  store i32 %349, i32* %351, align 4, !tbaa !35
  %352 = or i64 %26, 41
  %353 = shl i64 %indvars.iv, 32
  %sext41 = add i64 %353, 352187318272
  %354 = ashr exact i64 %sext41, 32
  %355 = getelementptr inbounds float, float* %7, i64 %354
  %356 = bitcast float* %355 to i32*
  %357 = load i32, i32* %356, align 4, !tbaa !32
  %358 = getelementptr inbounds float, float* %4, i64 %352
  %359 = bitcast float* %358 to i32*
  store i32 %357, i32* %359, align 4, !tbaa !35
  %360 = or i64 %26, 42
  %361 = shl i64 %indvars.iv, 32
  %sext42 = add i64 %361, 360777252864
  %362 = ashr exact i64 %sext42, 32
  %363 = getelementptr inbounds float, float* %7, i64 %362
  %364 = bitcast float* %363 to i32*
  %365 = load i32, i32* %364, align 4, !tbaa !32
  %366 = getelementptr inbounds float, float* %4, i64 %360
  %367 = bitcast float* %366 to i32*
  store i32 %365, i32* %367, align 4, !tbaa !35
  %368 = or i64 %26, 43
  %369 = shl i64 %indvars.iv, 32
  %sext43 = add i64 %369, 369367187456
  %370 = ashr exact i64 %sext43, 32
  %371 = getelementptr inbounds float, float* %7, i64 %370
  %372 = bitcast float* %371 to i32*
  %373 = load i32, i32* %372, align 4, !tbaa !32
  %374 = getelementptr inbounds float, float* %4, i64 %368
  %375 = bitcast float* %374 to i32*
  store i32 %373, i32* %375, align 4, !tbaa !35
  %376 = or i64 %26, 44
  %377 = shl i64 %indvars.iv, 32
  %sext44 = add i64 %377, 377957122048
  %378 = ashr exact i64 %sext44, 32
  %379 = getelementptr inbounds float, float* %7, i64 %378
  %380 = bitcast float* %379 to i32*
  %381 = load i32, i32* %380, align 4, !tbaa !32
  %382 = getelementptr inbounds float, float* %4, i64 %376
  %383 = bitcast float* %382 to i32*
  store i32 %381, i32* %383, align 4, !tbaa !35
  %384 = or i64 %26, 45
  %385 = shl i64 %indvars.iv, 32
  %sext45 = add i64 %385, 386547056640
  %386 = ashr exact i64 %sext45, 32
  %387 = getelementptr inbounds float, float* %7, i64 %386
  %388 = bitcast float* %387 to i32*
  %389 = load i32, i32* %388, align 4, !tbaa !32
  %390 = getelementptr inbounds float, float* %4, i64 %384
  %391 = bitcast float* %390 to i32*
  store i32 %389, i32* %391, align 4, !tbaa !35
  %392 = or i64 %26, 46
  %393 = shl i64 %indvars.iv, 32
  %sext46 = add i64 %393, 395136991232
  %394 = ashr exact i64 %sext46, 32
  %395 = getelementptr inbounds float, float* %7, i64 %394
  %396 = bitcast float* %395 to i32*
  %397 = load i32, i32* %396, align 4, !tbaa !32
  %398 = getelementptr inbounds float, float* %4, i64 %392
  %399 = bitcast float* %398 to i32*
  store i32 %397, i32* %399, align 4, !tbaa !35
  %400 = or i64 %26, 47
  %401 = shl i64 %indvars.iv, 32
  %sext47 = add i64 %401, 403726925824
  %402 = ashr exact i64 %sext47, 32
  %403 = getelementptr inbounds float, float* %7, i64 %402
  %404 = bitcast float* %403 to i32*
  %405 = load i32, i32* %404, align 4, !tbaa !32
  %406 = getelementptr inbounds float, float* %4, i64 %400
  %407 = bitcast float* %406 to i32*
  store i32 %405, i32* %407, align 4, !tbaa !35
  %408 = or i64 %26, 48
  %409 = shl i64 %indvars.iv, 32
  %sext67 = add i64 %409, 412316860416
  %410 = ashr exact i64 %sext67, 32
  %411 = getelementptr inbounds float, float* %7, i64 %410
  %412 = bitcast float* %411 to i32*
  %413 = load i32, i32* %412, align 4, !tbaa !32
  %414 = getelementptr inbounds float, float* %4, i64 %408
  %415 = bitcast float* %414 to i32*
  store i32 %413, i32* %415, align 4, !tbaa !35
  %416 = or i64 %26, 49
  %417 = shl i64 %indvars.iv, 32
  %sext48 = add i64 %417, 420906795008
  %418 = ashr exact i64 %sext48, 32
  %419 = getelementptr inbounds float, float* %7, i64 %418
  %420 = bitcast float* %419 to i32*
  %421 = load i32, i32* %420, align 4, !tbaa !32
  %422 = getelementptr inbounds float, float* %4, i64 %416
  %423 = bitcast float* %422 to i32*
  store i32 %421, i32* %423, align 4, !tbaa !35
  %424 = or i64 %26, 50
  %425 = shl i64 %indvars.iv, 32
  %sext49 = add i64 %425, 429496729600
  %426 = ashr exact i64 %sext49, 32
  %427 = getelementptr inbounds float, float* %7, i64 %426
  %428 = bitcast float* %427 to i32*
  %429 = load i32, i32* %428, align 4, !tbaa !32
  %430 = getelementptr inbounds float, float* %4, i64 %424
  %431 = bitcast float* %430 to i32*
  store i32 %429, i32* %431, align 4, !tbaa !35
  %432 = or i64 %26, 51
  %433 = shl i64 %indvars.iv, 32
  %sext50 = add i64 %433, 438086664192
  %434 = ashr exact i64 %sext50, 32
  %435 = getelementptr inbounds float, float* %7, i64 %434
  %436 = bitcast float* %435 to i32*
  %437 = load i32, i32* %436, align 4, !tbaa !32
  %438 = getelementptr inbounds float, float* %4, i64 %432
  %439 = bitcast float* %438 to i32*
  store i32 %437, i32* %439, align 4, !tbaa !35
  %440 = or i64 %26, 52
  %441 = shl i64 %indvars.iv, 32
  %sext51 = add i64 %441, 446676598784
  %442 = ashr exact i64 %sext51, 32
  %443 = getelementptr inbounds float, float* %7, i64 %442
  %444 = bitcast float* %443 to i32*
  %445 = load i32, i32* %444, align 4, !tbaa !32
  %446 = getelementptr inbounds float, float* %4, i64 %440
  %447 = bitcast float* %446 to i32*
  store i32 %445, i32* %447, align 4, !tbaa !35
  %448 = or i64 %26, 53
  %449 = shl i64 %indvars.iv, 32
  %sext52 = add i64 %449, 455266533376
  %450 = ashr exact i64 %sext52, 32
  %451 = getelementptr inbounds float, float* %7, i64 %450
  %452 = bitcast float* %451 to i32*
  %453 = load i32, i32* %452, align 4, !tbaa !32
  %454 = getelementptr inbounds float, float* %4, i64 %448
  %455 = bitcast float* %454 to i32*
  store i32 %453, i32* %455, align 4, !tbaa !35
  %456 = or i64 %26, 54
  %457 = shl i64 %indvars.iv, 32
  %sext53 = add i64 %457, 463856467968
  %458 = ashr exact i64 %sext53, 32
  %459 = getelementptr inbounds float, float* %7, i64 %458
  %460 = bitcast float* %459 to i32*
  %461 = load i32, i32* %460, align 4, !tbaa !32
  %462 = getelementptr inbounds float, float* %4, i64 %456
  %463 = bitcast float* %462 to i32*
  store i32 %461, i32* %463, align 4, !tbaa !35
  %464 = or i64 %26, 55
  %465 = shl i64 %indvars.iv, 32
  %sext54 = add i64 %465, 472446402560
  %466 = ashr exact i64 %sext54, 32
  %467 = getelementptr inbounds float, float* %7, i64 %466
  %468 = bitcast float* %467 to i32*
  %469 = load i32, i32* %468, align 4, !tbaa !32
  %470 = getelementptr inbounds float, float* %4, i64 %464
  %471 = bitcast float* %470 to i32*
  store i32 %469, i32* %471, align 4, !tbaa !35
  %472 = or i64 %26, 56
  %473 = shl i64 %indvars.iv, 32
  %sext68 = add i64 %473, 481036337152
  %474 = ashr exact i64 %sext68, 32
  %475 = getelementptr inbounds float, float* %7, i64 %474
  %476 = bitcast float* %475 to i32*
  %477 = load i32, i32* %476, align 4, !tbaa !32
  %478 = getelementptr inbounds float, float* %4, i64 %472
  %479 = bitcast float* %478 to i32*
  store i32 %477, i32* %479, align 4, !tbaa !35
  %480 = or i64 %26, 57
  %481 = shl i64 %indvars.iv, 32
  %sext55 = add i64 %481, 489626271744
  %482 = ashr exact i64 %sext55, 32
  %483 = getelementptr inbounds float, float* %7, i64 %482
  %484 = bitcast float* %483 to i32*
  %485 = load i32, i32* %484, align 4, !tbaa !32
  %486 = getelementptr inbounds float, float* %4, i64 %480
  %487 = bitcast float* %486 to i32*
  store i32 %485, i32* %487, align 4, !tbaa !35
  %488 = or i64 %26, 58
  %489 = shl i64 %indvars.iv, 32
  %sext56 = add i64 %489, 498216206336
  %490 = ashr exact i64 %sext56, 32
  %491 = getelementptr inbounds float, float* %7, i64 %490
  %492 = bitcast float* %491 to i32*
  %493 = load i32, i32* %492, align 4, !tbaa !32
  %494 = getelementptr inbounds float, float* %4, i64 %488
  %495 = bitcast float* %494 to i32*
  store i32 %493, i32* %495, align 4, !tbaa !35
  %496 = or i64 %26, 59
  %497 = shl i64 %indvars.iv, 32
  %sext57 = add i64 %497, 506806140928
  %498 = ashr exact i64 %sext57, 32
  %499 = getelementptr inbounds float, float* %7, i64 %498
  %500 = bitcast float* %499 to i32*
  %501 = load i32, i32* %500, align 4, !tbaa !32
  %502 = getelementptr inbounds float, float* %4, i64 %496
  %503 = bitcast float* %502 to i32*
  store i32 %501, i32* %503, align 4, !tbaa !35
  %504 = or i64 %26, 60
  %505 = shl i64 %indvars.iv, 32
  %sext58 = add i64 %505, 515396075520
  %506 = ashr exact i64 %sext58, 32
  %507 = getelementptr inbounds float, float* %7, i64 %506
  %508 = bitcast float* %507 to i32*
  %509 = load i32, i32* %508, align 4, !tbaa !32
  %510 = getelementptr inbounds float, float* %4, i64 %504
  %511 = bitcast float* %510 to i32*
  store i32 %509, i32* %511, align 4, !tbaa !35
  %512 = or i64 %26, 61
  %513 = shl i64 %indvars.iv, 32
  %sext59 = add i64 %513, 523986010112
  %514 = ashr exact i64 %sext59, 32
  %515 = getelementptr inbounds float, float* %7, i64 %514
  %516 = bitcast float* %515 to i32*
  %517 = load i32, i32* %516, align 4, !tbaa !32
  %518 = getelementptr inbounds float, float* %4, i64 %512
  %519 = bitcast float* %518 to i32*
  store i32 %517, i32* %519, align 4, !tbaa !35
  %520 = or i64 %26, 62
  %521 = shl i64 %indvars.iv, 32
  %sext60 = add i64 %521, 532575944704
  %522 = ashr exact i64 %sext60, 32
  %523 = getelementptr inbounds float, float* %7, i64 %522
  %524 = bitcast float* %523 to i32*
  %525 = load i32, i32* %524, align 4, !tbaa !32
  %526 = getelementptr inbounds float, float* %4, i64 %520
  %527 = bitcast float* %526 to i32*
  store i32 %525, i32* %527, align 4, !tbaa !35
  %528 = or i64 %26, 63
  %529 = shl i64 %indvars.iv, 32
  %sext61 = add i64 %529, 541165879296
  %530 = ashr exact i64 %sext61, 32
  %531 = getelementptr inbounds float, float* %7, i64 %530
  %532 = bitcast float* %531 to i32*
  %533 = load i32, i32* %532, align 4, !tbaa !32
  %534 = getelementptr inbounds float, float* %4, i64 %528
  %535 = bitcast float* %534 to i32*
  store i32 %533, i32* %535, align 4, !tbaa !35
  %indvars.iv.next = add nsw i64 %indvars.iv, 1
  %536 = icmp slt i64 %indvars.iv.next, %23
  br i1 %536, label %for_body, label %for_end, !prof !19

for_end:                                          ; preds = %for_body, %entry
  ret i32 0
}

define dllexport i32 @fused_nn_contrib_conv2d_NCHWc(i8* noalias nocapture readonly, i8* noalias nocapture readnone, i32) local_unnamed_addr !dbg !38 {
entry:
  call void @llvm.dbg.value(metadata i8* %0, metadata !40, metadata !DIExpression()), !dbg !43
  call void @llvm.dbg.value(metadata i8* %1, metadata !41, metadata !DIExpression()), !dbg !43
  call void @llvm.dbg.value(metadata i32 %2, metadata !42, metadata !DIExpression()), !dbg !43
  %3 = bitcast i8* %0 to %1**, !dbg !43
  %4 = load %1*, %1** %3, align 8, !dbg !43
  %5 = getelementptr inbounds i8, i8* %0, i64 8, !dbg !43
  %6 = bitcast i8* %5 to %1**, !dbg !43
  %7 = load %1*, %1** %6, align 8, !dbg !43
  %8 = getelementptr inbounds i8, i8* %0, i64 16, !dbg !43
  %9 = bitcast i8* %8 to %1**, !dbg !43
  %10 = load %1*, %1** %9, align 8, !dbg !43
  %11 = getelementptr inbounds %1, %1* %4, i64 0, i32 0, !dbg !43
  %12 = load i8*, i8** %11, align 8, !dbg !43
  %13 = getelementptr inbounds %1, %1* %7, i64 0, i32 0, !dbg !43
  %14 = load i8*, i8** %13, align 8, !dbg !43
  %15 = getelementptr inbounds %1, %1* %10, i64 0, i32 0, !dbg !43
  %16 = load i8*, i8** %15, align 8, !dbg !43
  %17 = tail call fastcc i32 @fused_nn_contrib_conv2d_NCHWc_compute_(i8* %12, i8* %14, i8* %16), !dbg !43
  ret i32 %17, !dbg !43
}

; Function Attrs: noinline
define private fastcc i32 @fused_nn_contrib_conv2d_NCHWc_compute_(i8* noalias, i8* noalias, i8* noalias) unnamed_addr #0 {
entry:
  %3 = alloca [100 x float], align 16
  %4 = alloca %6, align 8
  %.sub = getelementptr inbounds [100 x float], [100 x float]* %3, i64 0, i64 0
  %5 = getelementptr inbounds %6, %6* %4, i64 0, i32 0
  store float* %.sub, float** %5, align 8
  %6 = getelementptr inbounds %6, %6* %4, i64 0, i32 1
  store i8* %0, i8** %6, align 8
  %7 = bitcast %6* %4 to i8*
  %8 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %9 = call i32 %8(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.2, i8* nonnull %7, i32 0)
  %10 = icmp eq i32 %9, 0
  br i1 %10, label %call_end, label %call_fail, !prof !19

call_fail:                                        ; preds = %call_end, %entry
  %merge = phi i32 [ %9, %entry ], [ %17, %call_end ]
  ret i32 %merge

call_end:                                         ; preds = %entry
  %11 = alloca %7, align 8
  %12 = getelementptr inbounds %7, %7* %11, i64 0, i32 0
  store float* %.sub, float** %12, align 8
  %13 = getelementptr inbounds %7, %7* %11, i64 0, i32 1
  store i8* %1, i8** %13, align 8
  %14 = getelementptr inbounds %7, %7* %11, i64 0, i32 2
  store i8* %2, i8** %14, align 8
  %15 = bitcast %7* %11 to i8*
  %16 = load i32 (i32 (i32, %0*, i8*)*, i8*, i32)*, i32 (i32 (i32, %0*, i8*)*, i8*, i32)** @__TVMBackendParallelLaunch, align 8, !tbaa !16
  %17 = call i32 %16(i32 (i32, %0*, i8*)* nonnull @__tvm_parallel_lambda.3, i8* nonnull %15, i32 0)
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
  %10 = add nsw i32 %9, 9
  %11 = sdiv i32 %10, %9
  %12 = add nsw i32 %0, 1
  %13 = mul nsw i32 %11, %12
  %14 = icmp slt i32 %13, 10
  %15 = select i1 %14, i32 %13, i32 10
  %16 = mul nsw i32 %11, %0
  %17 = icmp slt i32 %16, 10
  %18 = select i1 %17, i32 %16, i32 10
  %19 = icmp slt i32 %18, %15
  br i1 %19, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %20 = xor i32 %16, -1
  %21 = icmp sgt i32 %20, -11
  %smax = select i1 %21, i32 %20, i32 -11
  %22 = mul i32 %smax, -10
  %23 = add i32 %22, -10
  %24 = add i32 %18, 1
  %25 = sext i32 %24 to i64
  %26 = add nsw i64 %25, -1
  %27 = sext i32 %15 to i64
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_end3
  %indvars.iv = phi i64 [ %26, %for_body.lr.ph ], [ %indvars.iv.next, %for_end3 ]
  %indvar = phi i32 [ 0, %for_body.lr.ph ], [ %indvar.next, %for_end3 ]
  %28 = mul nsw i64 %indvars.iv, 10
  %29 = trunc i64 %indvars.iv to i32
  %.off = add i32 %29, -1
  %30 = icmp ult i32 %.off, 8
  %31 = shl i32 %29, 3
  br i1 %30, label %if_end.us.9, label %for_body.split

for_body.split:                                   ; preds = %for_body
  %32 = mul i32 %indvar, 10
  %33 = add i32 %23, %32
  %34 = sext i32 %33 to i64
  %scevgep = getelementptr float, float* %4, i64 %34
  %scevgep5 = bitcast float* %scevgep to i8*
  call void @llvm.memset.p0i8.i64(i8* %scevgep5, i8 0, i64 40, i32 4, i1 false)
  br label %for_end3

for_end:                                          ; preds = %for_end3, %entry
  ret i32 0

for_end3:                                         ; preds = %for_body.split, %if_end.us.9
  %indvars.iv.next = add nsw i64 %indvars.iv, 1
  %35 = icmp slt i64 %indvars.iv.next, %27
  %indvar.next = add nuw i32 %indvar, 1
  br i1 %35, label %for_body, label %for_end, !prof !19

if_end.us.9:                                      ; preds = %for_body
  %36 = getelementptr inbounds float, float* %4, i64 %28
  store float 0.000000e+00, float* %36, align 4, !tbaa !44
  %37 = or i64 %28, 1
  %38 = add i32 %31, -8
  %39 = sext i32 %38 to i64
  %40 = getelementptr inbounds float, float* %7, i64 %39
  %41 = bitcast float* %40 to i32*
  %42 = load i32, i32* %41, align 4, !tbaa !47
  %43 = getelementptr inbounds float, float* %4, i64 %37
  %44 = bitcast float* %43 to i32*
  store i32 %42, i32* %44, align 4, !tbaa !44
  %45 = add nsw i64 %28, 2
  %46 = add i32 %31, -7
  %47 = sext i32 %46 to i64
  %48 = getelementptr inbounds float, float* %7, i64 %47
  %49 = bitcast float* %48 to i32*
  %50 = load i32, i32* %49, align 4, !tbaa !47
  %51 = getelementptr inbounds float, float* %4, i64 %45
  %52 = bitcast float* %51 to i32*
  store i32 %50, i32* %52, align 4, !tbaa !44
  %53 = add nsw i64 %28, 3
  %54 = add i32 %31, -6
  %55 = sext i32 %54 to i64
  %56 = getelementptr inbounds float, float* %7, i64 %55
  %57 = bitcast float* %56 to i32*
  %58 = load i32, i32* %57, align 4, !tbaa !47
  %59 = getelementptr inbounds float, float* %4, i64 %53
  %60 = bitcast float* %59 to i32*
  store i32 %58, i32* %60, align 4, !tbaa !44
  %61 = add nsw i64 %28, 4
  %62 = add i32 %31, -5
  %63 = sext i32 %62 to i64
  %64 = getelementptr inbounds float, float* %7, i64 %63
  %65 = bitcast float* %64 to i32*
  %66 = load i32, i32* %65, align 4, !tbaa !47
  %67 = getelementptr inbounds float, float* %4, i64 %61
  %68 = bitcast float* %67 to i32*
  store i32 %66, i32* %68, align 4, !tbaa !44
  %69 = add nsw i64 %28, 5
  %70 = add i32 %31, -4
  %71 = sext i32 %70 to i64
  %72 = getelementptr inbounds float, float* %7, i64 %71
  %73 = bitcast float* %72 to i32*
  %74 = load i32, i32* %73, align 4, !tbaa !47
  %75 = getelementptr inbounds float, float* %4, i64 %69
  %76 = bitcast float* %75 to i32*
  store i32 %74, i32* %76, align 4, !tbaa !44
  %77 = add nsw i64 %28, 6
  %78 = add i32 %31, -3
  %79 = sext i32 %78 to i64
  %80 = getelementptr inbounds float, float* %7, i64 %79
  %81 = bitcast float* %80 to i32*
  %82 = load i32, i32* %81, align 4, !tbaa !47
  %83 = getelementptr inbounds float, float* %4, i64 %77
  %84 = bitcast float* %83 to i32*
  store i32 %82, i32* %84, align 4, !tbaa !44
  %85 = add nsw i64 %28, 7
  %86 = add i32 %31, -2
  %87 = sext i32 %86 to i64
  %88 = getelementptr inbounds float, float* %7, i64 %87
  %89 = bitcast float* %88 to i32*
  %90 = load i32, i32* %89, align 4, !tbaa !47
  %91 = getelementptr inbounds float, float* %4, i64 %85
  %92 = bitcast float* %91 to i32*
  store i32 %90, i32* %92, align 4, !tbaa !44
  %93 = add nsw i64 %28, 8
  %94 = add i32 %31, -1
  %95 = sext i32 %94 to i64
  %96 = getelementptr inbounds float, float* %7, i64 %95
  %97 = bitcast float* %96 to i32*
  %98 = load i32, i32* %97, align 4, !tbaa !47
  %99 = getelementptr inbounds float, float* %4, i64 %93
  %100 = bitcast float* %99 to i32*
  store i32 %98, i32* %100, align 4, !tbaa !44
  %101 = add nsw i64 %28, 9
  %102 = getelementptr inbounds float, float* %4, i64 %101
  store float 0.000000e+00, float* %102, align 4, !tbaa !44
  br label %for_end3
}

; Function Attrs: nounwind
define private i32 @__tvm_parallel_lambda.3(i32, %0* nocapture readonly, i8* nocapture readonly) #2 {
entry:
  %3 = alloca [8 x <2 x float>], align 16
  %4 = bitcast [8 x <2 x float>]* %3 to i8*
  %.sub = getelementptr inbounds [8 x <2 x float>], [8 x <2 x float>]* %3, i64 0, i64 0
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
  %15 = add nsw i32 %14, 7
  %16 = sdiv i32 %15, %14
  %17 = add nsw i32 %0, 1
  %18 = mul nsw i32 %16, %17
  %19 = icmp slt i32 %18, 8
  %20 = select i1 %19, i32 %18, i32 8
  %21 = mul nsw i32 %16, %0
  %22 = icmp slt i32 %21, 8
  %23 = select i1 %22, i32 %21, i32 8
  %24 = icmp slt i32 %23, %20
  br i1 %24, label %for_body.lr.ph, label %for_end, !prof !19

for_body.lr.ph:                                   ; preds = %entry
  %25 = getelementptr inbounds [8 x <2 x float>], [8 x <2 x float>]* %3, i64 0, i64 0, i64 2
  %26 = bitcast float* %25 to <2 x float>*
  %27 = getelementptr inbounds [8 x <2 x float>], [8 x <2 x float>]* %3, i64 0, i64 0, i64 4
  %28 = bitcast float* %27 to <2 x float>*
  %29 = getelementptr inbounds [8 x <2 x float>], [8 x <2 x float>]* %3, i64 0, i64 0, i64 6
  %30 = bitcast float* %29 to <2 x float>*
  %31 = getelementptr inbounds [8 x <2 x float>], [8 x <2 x float>]* %3, i64 0, i64 0, i64 8
  %32 = bitcast float* %31 to <2 x float>*
  %33 = getelementptr inbounds [8 x <2 x float>], [8 x <2 x float>]* %3, i64 0, i64 0, i64 10
  %34 = bitcast float* %33 to <2 x float>*
  %35 = getelementptr inbounds [8 x <2 x float>], [8 x <2 x float>]* %3, i64 0, i64 0, i64 12
  %36 = bitcast float* %35 to <2 x float>*
  %37 = getelementptr inbounds [8 x <2 x float>], [8 x <2 x float>]* %3, i64 0, i64 0, i64 14
  %38 = bitcast float* %37 to <2 x float>*
  %39 = xor i32 %21, -1
  %40 = icmp sgt i32 %39, -9
  %smax = select i1 %40, i32 %39, i32 -9
  %41 = shl i32 %smax, 4
  %42 = sub i32 -16, %41
  %43 = add i32 %23, 1
  %44 = sext i32 %43 to i64
  %45 = add nsw i64 %44, -1
  %46 = sext i32 %20 to i64
  %47 = bitcast [8 x <2 x float>]* %3 to i8*
  %48 = bitcast float* %9 to <2 x float>*
  %49 = getelementptr inbounds float, float* %9, i64 2
  %50 = bitcast float* %49 to <2 x float>*
  %51 = getelementptr inbounds float, float* %9, i64 4
  %52 = bitcast float* %51 to <2 x float>*
  %53 = getelementptr inbounds float, float* %9, i64 6
  %54 = bitcast float* %53 to <2 x float>*
  %55 = getelementptr inbounds float, float* %9, i64 8
  %56 = bitcast float* %55 to <2 x float>*
  %57 = getelementptr inbounds float, float* %9, i64 10
  %58 = bitcast float* %57 to <2 x float>*
  %59 = getelementptr inbounds float, float* %9, i64 12
  %60 = bitcast float* %59 to <2 x float>*
  %61 = getelementptr inbounds float, float* %9, i64 14
  %62 = bitcast float* %61 to <2 x float>*
  %63 = getelementptr inbounds float, float* %9, i64 16
  %64 = bitcast float* %63 to <2 x float>*
  br label %for_body

for_body:                                         ; preds = %for_body.lr.ph, %for_body
  %indvars.iv49 = phi i64 [ %45, %for_body.lr.ph ], [ %indvars.iv.next50, %for_body ]
  %indvar = phi i32 [ 0, %for_body.lr.ph ], [ %indvar.next, %for_body ]
  call void @llvm.memset.p0i8.i64(i8* nonnull %47, i8 0, i64 64, i32 16, i1 false)
  %65 = shl i32 %indvar, 4
  %66 = add i32 %42, %65
  %67 = sext i32 %66 to i64
  %scevgep = getelementptr float, float* %12, i64 %67
  %scevgep48 = bitcast float* %scevgep to i8*
  %sext = mul i64 %indvars.iv49, 42949672960
  %68 = ashr exact i64 %sext, 32
  %69 = getelementptr inbounds float, float* %6, i64 %68
  %70 = load float, float* %69, align 4, !tbaa !44
  %71 = insertelement <2 x float> undef, float %70, i32 0
  %72 = shufflevector <2 x float> %71, <2 x float> undef, <2 x i32> zeroinitializer
  %73 = load <2 x float>, <2 x float>* %48, align 8, !tbaa !50
  %74 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %72, <2 x float> %73, <2 x float> zeroinitializer)
  %75 = or i64 %68, 1
  %76 = getelementptr inbounds float, float* %6, i64 %75
  %77 = load float, float* %76, align 4, !tbaa !44
  %78 = insertelement <2 x float> undef, float %77, i32 0
  %79 = shufflevector <2 x float> %78, <2 x float> undef, <2 x i32> zeroinitializer
  %80 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %79, <2 x float> %73, <2 x float> zeroinitializer)
  %81 = add nsw i64 %68, 2
  %82 = getelementptr inbounds float, float* %6, i64 %81
  %83 = load float, float* %82, align 4, !tbaa !44
  %84 = insertelement <2 x float> undef, float %83, i32 0
  %85 = shufflevector <2 x float> %84, <2 x float> undef, <2 x i32> zeroinitializer
  %86 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %85, <2 x float> %73, <2 x float> zeroinitializer)
  %87 = add nsw i64 %68, 3
  %88 = getelementptr inbounds float, float* %6, i64 %87
  %89 = load float, float* %88, align 4, !tbaa !44
  %90 = insertelement <2 x float> undef, float %89, i32 0
  %91 = shufflevector <2 x float> %90, <2 x float> undef, <2 x i32> zeroinitializer
  %92 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %91, <2 x float> %73, <2 x float> zeroinitializer)
  %93 = add nsw i64 %68, 4
  %94 = getelementptr inbounds float, float* %6, i64 %93
  %95 = load float, float* %94, align 4, !tbaa !44
  %96 = insertelement <2 x float> undef, float %95, i32 0
  %97 = shufflevector <2 x float> %96, <2 x float> undef, <2 x i32> zeroinitializer
  %98 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %97, <2 x float> %73, <2 x float> zeroinitializer)
  %99 = add nsw i64 %68, 5
  %100 = getelementptr inbounds float, float* %6, i64 %99
  %101 = load float, float* %100, align 4, !tbaa !44
  %102 = insertelement <2 x float> undef, float %101, i32 0
  %103 = shufflevector <2 x float> %102, <2 x float> undef, <2 x i32> zeroinitializer
  %104 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %103, <2 x float> %73, <2 x float> zeroinitializer)
  %105 = add nsw i64 %68, 6
  %106 = getelementptr inbounds float, float* %6, i64 %105
  %107 = load float, float* %106, align 4, !tbaa !44
  %108 = insertelement <2 x float> undef, float %107, i32 0
  %109 = shufflevector <2 x float> %108, <2 x float> undef, <2 x i32> zeroinitializer
  %110 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %109, <2 x float> %73, <2 x float> zeroinitializer)
  %111 = add nsw i64 %68, 7
  %112 = getelementptr inbounds float, float* %6, i64 %111
  %113 = load float, float* %112, align 4, !tbaa !44
  %114 = insertelement <2 x float> undef, float %113, i32 0
  %115 = shufflevector <2 x float> %114, <2 x float> undef, <2 x i32> zeroinitializer
  %116 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %115, <2 x float> %73, <2 x float> zeroinitializer)
  %117 = load <2 x float>, <2 x float>* %50, align 8, !tbaa !50
  %118 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %79, <2 x float> %117, <2 x float> %74)
  %119 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %85, <2 x float> %117, <2 x float> %80)
  %120 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %91, <2 x float> %117, <2 x float> %86)
  %121 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %97, <2 x float> %117, <2 x float> %92)
  %122 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %103, <2 x float> %117, <2 x float> %98)
  %123 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %109, <2 x float> %117, <2 x float> %104)
  %124 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %115, <2 x float> %117, <2 x float> %110)
  %125 = add nsw i64 %75, 7
  %126 = getelementptr inbounds float, float* %6, i64 %125
  %127 = load float, float* %126, align 4, !tbaa !44
  %128 = insertelement <2 x float> undef, float %127, i32 0
  %129 = shufflevector <2 x float> %128, <2 x float> undef, <2 x i32> zeroinitializer
  %130 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %129, <2 x float> %117, <2 x float> %116)
  %131 = load <2 x float>, <2 x float>* %52, align 8, !tbaa !50
  %132 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %85, <2 x float> %131, <2 x float> %118)
  %133 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %91, <2 x float> %131, <2 x float> %119)
  %134 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %97, <2 x float> %131, <2 x float> %120)
  %135 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %103, <2 x float> %131, <2 x float> %121)
  %136 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %109, <2 x float> %131, <2 x float> %122)
  %137 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %115, <2 x float> %131, <2 x float> %123)
  %138 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %129, <2 x float> %131, <2 x float> %124)
  %139 = add nsw i64 %68, 9
  %140 = getelementptr inbounds float, float* %6, i64 %139
  %141 = load float, float* %140, align 4, !tbaa !44
  %142 = insertelement <2 x float> undef, float %141, i32 0
  %143 = shufflevector <2 x float> %142, <2 x float> undef, <2 x i32> zeroinitializer
  %144 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %143, <2 x float> %131, <2 x float> %130)
  %145 = mul i64 %indvars.iv49, 42949672960
  %sext.1 = add i64 %145, 42949672960
  %146 = ashr exact i64 %sext.1, 32
  %147 = getelementptr inbounds float, float* %6, i64 %146
  %148 = load float, float* %147, align 4, !tbaa !44
  %149 = insertelement <2 x float> undef, float %148, i32 0
  %150 = shufflevector <2 x float> %149, <2 x float> undef, <2 x i32> zeroinitializer
  %151 = load <2 x float>, <2 x float>* %54, align 8, !tbaa !50
  %152 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %150, <2 x float> %151, <2 x float> %132)
  %153 = or i64 %146, 1
  %154 = getelementptr inbounds float, float* %6, i64 %153
  %155 = load float, float* %154, align 4, !tbaa !44
  %156 = insertelement <2 x float> undef, float %155, i32 0
  %157 = shufflevector <2 x float> %156, <2 x float> undef, <2 x i32> zeroinitializer
  %158 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %157, <2 x float> %151, <2 x float> %133)
  %159 = add nsw i64 %146, 2
  %160 = getelementptr inbounds float, float* %6, i64 %159
  %161 = load float, float* %160, align 4, !tbaa !44
  %162 = insertelement <2 x float> undef, float %161, i32 0
  %163 = shufflevector <2 x float> %162, <2 x float> undef, <2 x i32> zeroinitializer
  %164 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %163, <2 x float> %151, <2 x float> %134)
  %165 = add nsw i64 %146, 3
  %166 = getelementptr inbounds float, float* %6, i64 %165
  %167 = load float, float* %166, align 4, !tbaa !44
  %168 = insertelement <2 x float> undef, float %167, i32 0
  %169 = shufflevector <2 x float> %168, <2 x float> undef, <2 x i32> zeroinitializer
  %170 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %169, <2 x float> %151, <2 x float> %135)
  %171 = add nsw i64 %146, 4
  %172 = getelementptr inbounds float, float* %6, i64 %171
  %173 = load float, float* %172, align 4, !tbaa !44
  %174 = insertelement <2 x float> undef, float %173, i32 0
  %175 = shufflevector <2 x float> %174, <2 x float> undef, <2 x i32> zeroinitializer
  %176 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %175, <2 x float> %151, <2 x float> %136)
  %177 = add nsw i64 %146, 5
  %178 = getelementptr inbounds float, float* %6, i64 %177
  %179 = load float, float* %178, align 4, !tbaa !44
  %180 = insertelement <2 x float> undef, float %179, i32 0
  %181 = shufflevector <2 x float> %180, <2 x float> undef, <2 x i32> zeroinitializer
  %182 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %181, <2 x float> %151, <2 x float> %137)
  %183 = add nsw i64 %146, 6
  %184 = getelementptr inbounds float, float* %6, i64 %183
  %185 = load float, float* %184, align 4, !tbaa !44
  %186 = insertelement <2 x float> undef, float %185, i32 0
  %187 = shufflevector <2 x float> %186, <2 x float> undef, <2 x i32> zeroinitializer
  %188 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %187, <2 x float> %151, <2 x float> %138)
  %189 = add nsw i64 %146, 7
  %190 = getelementptr inbounds float, float* %6, i64 %189
  %191 = load float, float* %190, align 4, !tbaa !44
  %192 = insertelement <2 x float> undef, float %191, i32 0
  %193 = shufflevector <2 x float> %192, <2 x float> undef, <2 x i32> zeroinitializer
  %194 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %193, <2 x float> %151, <2 x float> %144)
  %195 = load <2 x float>, <2 x float>* %56, align 8, !tbaa !50
  %196 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %157, <2 x float> %195, <2 x float> %152)
  %197 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %163, <2 x float> %195, <2 x float> %158)
  %198 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %169, <2 x float> %195, <2 x float> %164)
  %199 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %175, <2 x float> %195, <2 x float> %170)
  %200 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %181, <2 x float> %195, <2 x float> %176)
  %201 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %187, <2 x float> %195, <2 x float> %182)
  %202 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %193, <2 x float> %195, <2 x float> %188)
  %203 = add nsw i64 %153, 7
  %204 = getelementptr inbounds float, float* %6, i64 %203
  %205 = load float, float* %204, align 4, !tbaa !44
  %206 = insertelement <2 x float> undef, float %205, i32 0
  %207 = shufflevector <2 x float> %206, <2 x float> undef, <2 x i32> zeroinitializer
  %208 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %207, <2 x float> %195, <2 x float> %194)
  %209 = load <2 x float>, <2 x float>* %58, align 8, !tbaa !50
  %210 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %163, <2 x float> %209, <2 x float> %196)
  %211 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %169, <2 x float> %209, <2 x float> %197)
  %212 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %175, <2 x float> %209, <2 x float> %198)
  %213 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %181, <2 x float> %209, <2 x float> %199)
  %214 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %187, <2 x float> %209, <2 x float> %200)
  %215 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %193, <2 x float> %209, <2 x float> %201)
  %216 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %207, <2 x float> %209, <2 x float> %202)
  %217 = add nsw i64 %146, 9
  %218 = getelementptr inbounds float, float* %6, i64 %217
  %219 = load float, float* %218, align 4, !tbaa !44
  %220 = insertelement <2 x float> undef, float %219, i32 0
  %221 = shufflevector <2 x float> %220, <2 x float> undef, <2 x i32> zeroinitializer
  %222 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %221, <2 x float> %209, <2 x float> %208)
  %223 = mul i64 %indvars.iv49, 42949672960
  %sext.2 = add i64 %223, 85899345920
  %224 = ashr exact i64 %sext.2, 32
  %225 = getelementptr inbounds float, float* %6, i64 %224
  %226 = load float, float* %225, align 4, !tbaa !44
  %227 = insertelement <2 x float> undef, float %226, i32 0
  %228 = shufflevector <2 x float> %227, <2 x float> undef, <2 x i32> zeroinitializer
  %229 = load <2 x float>, <2 x float>* %60, align 8, !tbaa !50
  %230 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %228, <2 x float> %229, <2 x float> %210)
  %231 = or i64 %224, 1
  %232 = getelementptr inbounds float, float* %6, i64 %231
  %233 = load float, float* %232, align 4, !tbaa !44
  %234 = insertelement <2 x float> undef, float %233, i32 0
  %235 = shufflevector <2 x float> %234, <2 x float> undef, <2 x i32> zeroinitializer
  %236 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %235, <2 x float> %229, <2 x float> %211)
  %237 = add nsw i64 %224, 2
  %238 = getelementptr inbounds float, float* %6, i64 %237
  %239 = load float, float* %238, align 4, !tbaa !44
  %240 = insertelement <2 x float> undef, float %239, i32 0
  %241 = shufflevector <2 x float> %240, <2 x float> undef, <2 x i32> zeroinitializer
  %242 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %241, <2 x float> %229, <2 x float> %212)
  %243 = add nsw i64 %224, 3
  %244 = getelementptr inbounds float, float* %6, i64 %243
  %245 = load float, float* %244, align 4, !tbaa !44
  %246 = insertelement <2 x float> undef, float %245, i32 0
  %247 = shufflevector <2 x float> %246, <2 x float> undef, <2 x i32> zeroinitializer
  %248 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %247, <2 x float> %229, <2 x float> %213)
  %249 = add nsw i64 %224, 4
  %250 = getelementptr inbounds float, float* %6, i64 %249
  %251 = load float, float* %250, align 4, !tbaa !44
  %252 = insertelement <2 x float> undef, float %251, i32 0
  %253 = shufflevector <2 x float> %252, <2 x float> undef, <2 x i32> zeroinitializer
  %254 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %253, <2 x float> %229, <2 x float> %214)
  %255 = add nsw i64 %224, 5
  %256 = getelementptr inbounds float, float* %6, i64 %255
  %257 = load float, float* %256, align 4, !tbaa !44
  %258 = insertelement <2 x float> undef, float %257, i32 0
  %259 = shufflevector <2 x float> %258, <2 x float> undef, <2 x i32> zeroinitializer
  %260 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %259, <2 x float> %229, <2 x float> %215)
  %261 = add nsw i64 %224, 6
  %262 = getelementptr inbounds float, float* %6, i64 %261
  %263 = load float, float* %262, align 4, !tbaa !44
  %264 = insertelement <2 x float> undef, float %263, i32 0
  %265 = shufflevector <2 x float> %264, <2 x float> undef, <2 x i32> zeroinitializer
  %266 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %265, <2 x float> %229, <2 x float> %216)
  %267 = add nsw i64 %224, 7
  %268 = getelementptr inbounds float, float* %6, i64 %267
  %269 = load float, float* %268, align 4, !tbaa !44
  %270 = insertelement <2 x float> undef, float %269, i32 0
  %271 = shufflevector <2 x float> %270, <2 x float> undef, <2 x i32> zeroinitializer
  %272 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %271, <2 x float> %229, <2 x float> %222)
  %273 = load <2 x float>, <2 x float>* %62, align 8, !tbaa !50
  %274 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %235, <2 x float> %273, <2 x float> %230)
  %275 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %241, <2 x float> %273, <2 x float> %236)
  %276 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %247, <2 x float> %273, <2 x float> %242)
  %277 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %253, <2 x float> %273, <2 x float> %248)
  %278 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %259, <2 x float> %273, <2 x float> %254)
  %279 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %265, <2 x float> %273, <2 x float> %260)
  %280 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %271, <2 x float> %273, <2 x float> %266)
  %281 = add nsw i64 %231, 7
  %282 = getelementptr inbounds float, float* %6, i64 %281
  %283 = load float, float* %282, align 4, !tbaa !44
  %284 = insertelement <2 x float> undef, float %283, i32 0
  %285 = shufflevector <2 x float> %284, <2 x float> undef, <2 x i32> zeroinitializer
  %286 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %285, <2 x float> %273, <2 x float> %272)
  %287 = load <2 x float>, <2 x float>* %64, align 8, !tbaa !50
  %288 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %241, <2 x float> %287, <2 x float> %274)
  %289 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %247, <2 x float> %287, <2 x float> %275)
  %290 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %253, <2 x float> %287, <2 x float> %276)
  %291 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %259, <2 x float> %287, <2 x float> %277)
  %292 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %265, <2 x float> %287, <2 x float> %278)
  %293 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %271, <2 x float> %287, <2 x float> %279)
  %294 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %285, <2 x float> %287, <2 x float> %280)
  %295 = add nsw i64 %224, 9
  %296 = getelementptr inbounds float, float* %6, i64 %295
  %297 = load float, float* %296, align 4, !tbaa !44
  %298 = insertelement <2 x float> undef, float %297, i32 0
  %299 = shufflevector <2 x float> %298, <2 x float> undef, <2 x i32> zeroinitializer
  %300 = tail call <2 x float> @llvm.fmuladd.v2f32(<2 x float> %299, <2 x float> %287, <2 x float> %286)
  store <2 x float> %288, <2 x float>* %.sub, align 16, !tbaa !53
  store <2 x float> %289, <2 x float>* %26, align 8, !tbaa !53
  store <2 x float> %290, <2 x float>* %28, align 16, !tbaa !53
  store <2 x float> %291, <2 x float>* %30, align 8, !tbaa !53
  store <2 x float> %292, <2 x float>* %32, align 16, !tbaa !53
  store <2 x float> %293, <2 x float>* %34, align 8, !tbaa !53
  store <2 x float> %294, <2 x float>* %36, align 16, !tbaa !53
  store <2 x float> %300, <2 x float>* %38, align 8, !tbaa !53
  call void @llvm.memcpy.p0i8.p0i8.i64(i8* %scevgep48, i8* nonnull %4, i64 64, i32 8, i1 false)
  %indvars.iv.next50 = add nsw i64 %indvars.iv49, 1
  %301 = icmp slt i64 %indvars.iv.next50, %46
  %indvar.next = add nuw i32 %indvar, 1
  br i1 %301, label %for_body, label %for_end, !prof !19

for_end:                                          ; preds = %for_body, %entry
  ret i32 0
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
!22 = !{!"0x5567734d69c0", !18, i64 0}
!23 = !{!24, !24, i64 0}
!24 = !{!"float32", !25, i64 0}
!25 = !{!"0x5567734d6a10", !18, i64 0}
!26 = distinct !DISubprogram(name: "fused_layout_transform_1", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !27)
!27 = !{!28, !29, !30}
!28 = !DILocalVariable(name: "arg1", arg: 1, scope: !26, file: !1, type: !9)
!29 = !DILocalVariable(name: "arg2", arg: 2, scope: !26, file: !1, type: !9)
!30 = !DILocalVariable(name: "arg3", arg: 3, scope: !26, file: !1, type: !8)
!31 = !DILocation(line: 0, scope: !26)
!32 = !{!33, !33, i64 0}
!33 = !{!"float32", !34, i64 0}
!34 = !{!"0x5567734f3560", !18, i64 0}
!35 = !{!36, !36, i64 0}
!36 = !{!"float32", !37, i64 0}
!37 = !{!"0x55677341a730", !18, i64 0}
!38 = distinct !DISubprogram(name: "fused_nn_contrib_conv2d_NCHWc", scope: !1, file: !1, type: !6, isLocal: false, isDefinition: true, flags: DIFlagPrototyped, isOptimized: true, unit: !0, variables: !39)
!39 = !{!40, !41, !42}
!40 = !DILocalVariable(name: "arg1", arg: 1, scope: !38, file: !1, type: !9)
!41 = !DILocalVariable(name: "arg2", arg: 2, scope: !38, file: !1, type: !9)
!42 = !DILocalVariable(name: "arg3", arg: 3, scope: !38, file: !1, type: !8)
!43 = !DILocation(line: 0, scope: !38)
!44 = !{!45, !45, i64 0}
!45 = !{!"float32", !46, i64 0}
!46 = !{!"0x5567733f6ba0", !18, i64 0}
!47 = !{!48, !48, i64 0}
!48 = !{!"float32", !49, i64 0}
!49 = !{!"0x5567733d0450", !18, i64 0}
!50 = !{!51, !51, i64 0}
!51 = !{!"float32", !52, i64 0}
!52 = !{!"0x556773445150", !18, i64 0}
!53 = !{!54, !54, i64 0}
!54 = !{!"0x5567734e6ef0.w2.b0", !55, i64 0}
!55 = !{!"0x5567734e6ef0.w4.b0", !56, i64 0}
!56 = !{!"0x5567734e6ef0.w8.b0", !57, i64 0}
!57 = !{!"0x5567734e6ef0.w16.b0", !58, i64 0}
!58 = !{!"0x5567734e6ef0.w32.b0", !59, i64 0}
!59 = !{!"0x5567734e6ef0.w64.b0", !60, i64 0}
!60 = !{!"0x5567734e6ef0.w128.b0", !61, i64 0}
!61 = !{!"0x5567734e6ef0.w256.b0", !62, i64 0}
!62 = !{!"0x5567734e6ef0.w512.b0", !63, i64 0}
!63 = !{!"0x5567734e6ef0.w1024.b0", !64, i64 0}
!64 = !{!"float32", !65, i64 0}
!65 = !{!"0x5567734e6ef0", !18, i64 0}
