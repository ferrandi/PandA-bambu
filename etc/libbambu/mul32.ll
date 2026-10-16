; ModuleID = '../../../etc/libbambu/mul32.c'
source_filename = "../../../etc/libbambu/mul32.c"
target datalayout = "e-m:e-p:32:32-p270:32:32-p271:32:32-p272:64:64-f64:32:64-f80:32-n8:16:32-S128"
target triple = "i386-unknown-linux-gnu"

; Function Attrs: noinline nounwind optnone uwtable
define i32 @__umul32(i32, i32) local_unnamed_addr #0 {
  %3 = lshr i32 %0, 16
  %4 = lshr i32 %1, 16
  %5 = and i32 %0, 65535
  %6 = and i32 %1, 65535
  %7 = mul nuw i32 %6, %3
  %8 = mul nuw i32 %4, %5
  %9 = add i32 %7, %8
  %10 = shl i32 %9, 16
  %11 = mul nuw i32 %6, %5
  %12 = add i32 %10, %11
  ret i32 %12
}

; Function Attrs: noinline nounwind optnone uwtable
define i32 @__mul32(i32, i32) local_unnamed_addr #0 {
  %3 = lshr i32 %0, 16
  %4 = lshr i32 %1, 16
  %5 = and i32 %0, 65535
  %6 = and i32 %1, 65535
  %7 = mul nuw i32 %6, %3
  %8 = mul nuw i32 %4, %5
  %9 = add i32 %7, %8
  %10 = shl i32 %9, 16
  %11 = mul nuw i32 %6, %5
  %12 = add i32 %10, %11
  ret i32 %12
}

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-builtin-bcmp" "no-builtin-memcpy" "no-builtin-memmove" "no-builtin-memset" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="pentium4" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"NumRegisterParameters", i32 0}
!1 = !{i32 1, !"wchar_size", i32 4}
!2 = !{!"clang version 11.1.0 (/github/workspace/llvm-project/clang 1fdec59bffc11ae37eb51a1b9869f0696bfd5312)"}
