; Native LLVM golden: store <4 x i1> [1,1,0,1] as packed bits.
; The first two bytes are 11 and 0 on the i386 little-endian target.
target triple = "i386-pc-linux-gnu"

@bytes = global [16 x i8] zeroinitializer
@fmt = private constant [7 x i8] c"%d %d\0A\00"

declare i32 @printf(i8*, ...)

define void @storemask(i32 %x, i32 %y) #0 {
entry:
  %c0 = icmp eq i32 %x, %y
  %m0 = insertelement <4 x i1> undef, i1 %c0, i32 0
  %m1 = insertelement <4 x i1> %m0, i1 true, i32 1
  %m2 = insertelement <4 x i1> %m1, i1 false, i32 2
  %m3 = insertelement <4 x i1> %m2, i1 true, i32 3
  %maskptr = bitcast [16 x i8]* @bytes to <4 x i1>*
  store <4 x i1> %m3, <4 x i1>* %maskptr, align 1
  %p0 = getelementptr [16 x i8], [16 x i8]* @bytes, i32 0, i32 0
  %p1 = getelementptr [16 x i8], [16 x i8]* @bytes, i32 0, i32 1
  %b0 = load i8, i8* %p0, align 1
  %b1 = load i8, i8* %p1, align 1
  %z0 = zext i8 %b0 to i32
  %z1 = zext i8 %b1 to i32
  %fmtptr = getelementptr [7 x i8], [7 x i8]* @fmt, i32 0, i32 0
  %call = call i32 (i8*, ...) @printf(i8* %fmtptr, i32 %z0, i32 %z1)
  ret void
}

define i32 @main() #0 {
entry:
  call void @storemask(i32 5, i32 5)
  ret i32 0
}

; Protect the test instructions; a Function Attrs comment alone has no effect.
attributes #0 = { noinline optnone }
