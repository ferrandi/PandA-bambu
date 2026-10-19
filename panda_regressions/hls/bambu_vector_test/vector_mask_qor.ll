; Pointer-input N=16 fixture matching the recorded area/timing baseline.
target triple = "i386-pc-linux-gnu"

define i32 @eq16(<16 x i32>* %a, <16 x i32>* %b) #0 {
  %av = load <16 x i32>, <16 x i32>* %a, align 4
  %bv = load <16 x i32>, <16 x i32>* %b, align 4
  %cmp = icmp eq <16 x i32> %av, %bv
  %mask = bitcast <16 x i1> %cmp to i16
  %wide = zext i16 %mask to i32
  ret i32 %wide
}

; Protect the test instructions; a Function Attrs comment alone has no effect.
attributes #0 = { noinline optnone }
