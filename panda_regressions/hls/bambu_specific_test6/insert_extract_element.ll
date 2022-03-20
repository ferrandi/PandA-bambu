; ModuleID = 'test.c'
source_filename = "test.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define i16 @test1(i16 %0, i16 %1, i16 %2, i16 %3, i16 %4, i16 %5, i16 %6, i16 %7) #0 {
  %vec0 = insertelement <8 x i16> undef, i16 %0, i32 0
  %vec1 = insertelement <8 x i16> %vec0, i16 %1, i32 1
  %vec2 = insertelement <8 x i16> %vec1, i16 %2, i32 2
  %vec3 = insertelement <8 x i16> %vec2, i16 %3, i32 3
  %vec4 = insertelement <8 x i16> %vec3, i16 %4, i32 4
  %vec5 = insertelement <8 x i16> %vec4, i16 %5, i32 5
  %vec6 = insertelement <8 x i16> %vec5, i16 %6, i32 6
  %vec7 = insertelement <8 x i16> %vec6, i16 %7, i32 7
  %val = extractelement <8 x i16> %vec7, i32 5
  %vec = insertelement <1 x i16> undef, i16 %val, i32 0
  %res1 = extractelement <1 x i16> %vec, i32 0
  ret i16 %res1
}

define i16 @test2(i16 %0, i16 %1, i16 %2, i16 %3, i16 %4, i16 %5, i16 %6, i16 %7) #0 {
  %vec0 = insertelement <8 x i16> undef, i16 %0, i32 0
  %vec1 = insertelement <8 x i16> %vec0, i16 %1, i32 1
  %vec2 = insertelement <8 x i16> %vec1, i16 %2, i32 2
  %vec3 = insertelement <8 x i16> %vec2, i16 %3, i32 3
  %vec4 = insertelement <8 x i16> %vec3, i16 %4, i32 4
  %vec5 = insertelement <8 x i16> %vec4, i16 %5, i32 5
  %vec6 = insertelement <8 x i16> %vec5, i16 %6, i32 6
  %vec7 = insertelement <8 x i16> %vec6, i16 %7, i32 7
  %vec = shufflevector <8 x i16> %vec7, <8 x i16> undef, <1 x i32> <i32 5>
  %res1 = extractelement <1 x i16> %vec, i32 0
  ret i16 %res1
}


; Function Attrs: noinline nounwind optnone uwtable
define i16 @test3(i16 %0, i16 %1, i16 %2, i16 %3, i16 %4, i16 %5, i16 %6, i16 %7) #0 {
bb1:
  %vec0 = insertelement <8 x i16> undef, i16 %0, i32 0
  %vec1 = insertelement <8 x i16> %vec0, i16 %1, i32 1
  %vec2 = insertelement <8 x i16> %vec1, i16 %2, i32 2
  %vec3 = insertelement <8 x i16> %vec2, i16 %3, i32 3
  %vec4 = insertelement <8 x i16> %vec3, i16 %4, i32 4
  %vec5 = insertelement <8 x i16> %vec4, i16 %5, i32 5
  %vec6 = insertelement <8 x i16> %vec5, i16 %6, i32 6
  %vec7 = insertelement <8 x i16> %vec6, i16 %7, i32 7
  %c = icmp eq i16 %0, 0
  br i1 %c, label %bb2, label %bb3

bb2:                                              ; preds = %bb1
  %el1 = extractelement <8 x i16> %vec7, i32 3
  %el2 = add nsw i16 %el1, 1
  %vec8 = insertelement <8 x i16> %vec7, i16 %el2, i32 0
  br label %bb3

bb3:                                              ; preds = %bb2, %bb1
  %tmp1 = phi <8 x i16> [ %vec7, %bb1 ], [ %vec8, %bb2 ]
  %tmp2 = phi <8 x i16> [ %vec0, %bb1 ], [ zeroinitializer, %bb2 ]

  %res1 = extractelement <8 x i16> %tmp1, i32 0
  %res2 = extractelement <8 x i16> %tmp2, i32 0
  %res3 = add nsw i16 %res1, %res2
  ret i16 %res3
}

attributes #0 = { noinline nounwind optnone uwtable "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"Ubuntu clang version 12.0.0-3ubuntu1~20.04.4"}

