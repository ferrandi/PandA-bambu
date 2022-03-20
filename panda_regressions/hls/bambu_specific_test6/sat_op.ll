; ModuleID = 'sat_op.c'
source_filename = "sat_op.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i64 @usat_add64(i64 zeroext %0, i64 zeroext %1) local_unnamed_addr #1 {
  %3 = tail call i64 @llvm.uadd.sat.i64(i64 %0, i64 %1)
  ret i64 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local signext i64 @sat_add64(i64 signext %0, i64 signext %1) local_unnamed_addr #1 {
  %3 = tail call i64 @llvm.sadd.sat.i64(i64 %0, i64 %1)
  ret i64 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i64 @usat_sub64(i64 zeroext %0, i64 zeroext %1) local_unnamed_addr #1 {
  %3 = tail call i64 @llvm.usub.sat.i64(i64 %0, i64 %1)
  ret i64 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local signext i64 @sat_sub64(i64 signext %0, i64 signext %1) local_unnamed_addr #1 {
  %3 = tail call i64 @llvm.ssub.sat.i64(i64 %0, i64 %1)
  ret i64 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i32 @usat_add32(i32 zeroext %0, i32 zeroext %1) local_unnamed_addr #1 {
  %3 = tail call i32 @llvm.uadd.sat.i32(i32 %0, i32 %1)
  ret i32 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local signext i32 @sat_add32(i32 signext %0, i32 signext %1) local_unnamed_addr #1 {
  %3 = tail call i32 @llvm.sadd.sat.i32(i32 %0, i32 %1)
  ret i32 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i32 @usat_sub32(i32 zeroext %0, i32 zeroext %1) local_unnamed_addr #1 {
  %3 = tail call i32 @llvm.usub.sat.i32(i32 %0, i32 %1)
  ret i32 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local signext i32 @sat_sub32(i32 signext %0, i32 signext %1) local_unnamed_addr #1 {
  %3 = tail call i32 @llvm.ssub.sat.i32(i32 %0, i32 %1)
  ret i32 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i16 @usat_add16(i16 zeroext %0, i16 zeroext %1) local_unnamed_addr #1 {
  %3 = tail call i16 @llvm.uadd.sat.i16(i16 %0, i16 %1)
  ret i16 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local signext i16 @sat_add16(i16 signext %0, i16 signext %1) local_unnamed_addr #1 {
  %3 = tail call i16 @llvm.sadd.sat.i16(i16 %0, i16 %1)
  ret i16 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i16 @usat_sub16(i16 zeroext %0, i16 zeroext %1) local_unnamed_addr #1 {
  %3 = tail call i16 @llvm.usub.sat.i16(i16 %0, i16 %1)
  ret i16 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local signext i16 @sat_sub16(i16 signext %0, i16 signext %1) local_unnamed_addr #1 {
  %3 = tail call i16 @llvm.ssub.sat.i16(i16 %0, i16 %1)
  ret i16 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i8 @usat_add8(i8 zeroext %0, i8 zeroext %1) local_unnamed_addr #1 {
  %3 = tail call i8 @llvm.uadd.sat.i8(i8 %0, i8 %1)
  ret i8 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local signext i8 @sat_add8(i8 signext %0, i8 signext %1) local_unnamed_addr #1 {
  %3 = tail call i8 @llvm.sadd.sat.i8(i8 %0, i8 %1)
  ret i8 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local zeroext i8 @usat_sub8(i8 zeroext %0, i8 zeroext %1) local_unnamed_addr #1 {
  %3 = tail call i8 @llvm.usub.sat.i8(i8 %0, i8 %1)
  ret i8 %3
}

; Function Attrs: nounwind readnone uwtable
define dso_local signext i8 @sat_sub8(i8 signext %0, i8 signext %1) local_unnamed_addr #1 {
  %3 = tail call i8 @llvm.ssub.sat.i8(i8 %0, i8 %1)
  ret i8 %3
}

; Function Attrs: nounwind readnone speculatable willreturn
declare i64 @llvm.usub.sat.i64(i64, i64) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i64 @llvm.ssub.sat.i64(i64, i64) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i64 @llvm.uadd.sat.i64(i64, i64) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i64 @llvm.sadd.sat.i64(i64, i64) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i32 @llvm.usub.sat.i32(i32, i32) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i32 @llvm.ssub.sat.i32(i32, i32) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i32 @llvm.uadd.sat.i32(i32, i32) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i32 @llvm.sadd.sat.i32(i32, i32) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i16 @llvm.usub.sat.i16(i16, i16) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i16 @llvm.ssub.sat.i16(i16, i16) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i16 @llvm.uadd.sat.i16(i16, i16) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i16 @llvm.sadd.sat.i16(i16, i16) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i8 @llvm.usub.sat.i8(i8, i8) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i8 @llvm.ssub.sat.i8(i8, i8) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i8 @llvm.uadd.sat.i8(i8, i8) #2

; Function Attrs: nounwind readnone speculatable willreturn
declare i8 @llvm.sadd.sat.i8(i8, i8) #2

attributes #0 = { norecurse nounwind readnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { nounwind readnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="none" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readnone speculatable willreturn }

!llvm.module.flags = !{!0}
!llvm.ident = !{!1}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{!"clang version 11.0.0 (git@github.com:llvm/llvm-project.git 176249bd6732a8044d457092ed932768724a6f06)"}
