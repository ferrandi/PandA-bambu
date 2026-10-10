; ModuleID = 'etc/libbambu/mul64.c'
source_filename = "etc/libbambu/mul64.c"
target datalayout = "e-m:e-p:32:32-p270:32:32-p271:32:32-p272:64:64-f64:32:64-f80:32-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define i64 @__umul64(i64, i64) local_unnamed_addr #0 {
  %3 = lshr i64 %0, 32
  %4 = trunc i64 %3 to i32
  %5 = trunc i64 %0 to i32
  %6 = lshr i64 %1, 32
  %7 = trunc i64 %6 to i32
  %8 = trunc i64 %1 to i32
  %9 = and i32 %5, 65535
  %10 = lshr i32 %5, 16
  %11 = and i32 %8, 65535
  %12 = lshr i32 %8, 16
  %13 = mul nuw i32 %11, %9
  %14 = mul nuw i32 %11, %10
  %15 = mul nuw i32 %12, %9
  %16 = mul nuw i32 %12, %10
  %17 = zext i32 %14 to i64
  %18 = zext i32 %13 to i64
  %19 = zext i32 %16 to i64
  %20 = shl nuw nsw i64 %19, 16
  %21 = zext i32 %15 to i64
  %22 = add nuw nsw i64 %20, %17
  %23 = add nuw nsw i64 %21, %22
  %24 = shl nuw i64 %23, 16
  %25 = add nuw i64 %24, %18
  %26 = lshr i64 %25, 32
  %27 = trunc i64 %26 to i32
  %28 = mul i32 %7, %5
  %29 = add i32 %27, %28
  %30 = mul i32 %4, %8
  %31 = add i32 %29, %30
  %32 = zext i32 %31 to i64
  %33 = shl nuw i64 %32, 32
  %34 = and i64 %25, 4294967295
  %35 = or i64 %33, %34
  ret i64 %35
}

; Function Attrs: noinline nounwind optnone uwtable
define i64 @__mul64(i64, i64) local_unnamed_addr #0 {
  %3 = lshr i64 %0, 32
  %4 = trunc i64 %3 to i32
  %5 = trunc i64 %0 to i32
  %6 = lshr i64 %1, 32
  %7 = trunc i64 %6 to i32
  %8 = trunc i64 %1 to i32
  %9 = and i32 %5, 65535
  %10 = lshr i32 %5, 16
  %11 = and i32 %8, 65535
  %12 = lshr i32 %8, 16
  %13 = mul nuw i32 %11, %9
  %14 = mul nuw i32 %11, %10
  %15 = mul nuw i32 %12, %9
  %16 = mul nuw i32 %12, %10
  %17 = zext i32 %14 to i64
  %18 = zext i32 %13 to i64
  %19 = zext i32 %16 to i64
  %20 = shl nuw nsw i64 %19, 16
  %21 = zext i32 %15 to i64
  %22 = add nuw nsw i64 %20, %17
  %23 = add nuw nsw i64 %21, %22
  %24 = shl nuw i64 %23, 16
  %25 = add nuw i64 %24, %18
  %26 = lshr i64 %25, 32
  %27 = trunc i64 %26 to i32
  %28 = mul i32 %7, %5
  %29 = add i32 %27, %28
  %30 = mul i32 %4, %8
  %31 = add i32 %29, %30
  %32 = zext i32 %31 to i64
  %33 = shl nuw i64 %32, 32
  %34 = and i64 %25, 4294967295
  %35 = or i64 %33, %34
  ret i64 %35
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-builtin-bcmp" "no-builtin-memcpy" "no-builtin-memmove" "no-builtin-memset" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="pentium4" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"NumRegisterParameters", i32 0}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{!"clang version 11.1.0 (/github/workspace/llvm-project/clang 1fdec59bffc11ae37eb51a1b9869f0696bfd5312)"}
