; Fixed-vector mask checks using typed pointers and LLVM 6-compatible IR.
target triple = "i386-pc-linux-gnu"

; IPSCCP may fold constant instructions even with optnone. A local volatile
; zero keeps the mixed-lane literal and its bitcast in the frontend IR,
; without introducing a global memory argument. mask_const4 returns 0x0b.

define i8 @mask_eq8(i64 %a, i64 %b) #0 {
entry:
  %av = bitcast i64 %a to <8 x i8>
  %bv = bitcast i64 %b to <8 x i8>
  %cmp = icmp eq <8 x i8> %av, %bv
  %bits = bitcast <8 x i1> %cmp to i8
  ret i8 %bits
}

define i8 @mask_const4() #0 {
entry:
  %guard_slot = alloca i8, align 1
  store volatile i8 0, i8* %guard_slot, align 1
  %guard = load volatile i8, i8* %guard_slot, align 1
  %guard_bits = trunc i8 %guard to i4
  %guard_mask = bitcast i4 %guard_bits to <4 x i1>
  %mask = xor <4 x i1> %guard_mask, <i1 true, i1 true, i1 false, i1 true>
  %bits = bitcast <4 x i1> %mask to i4
  %wide = zext i4 %bits to i8
  ret i8 %wide
}

define i32 @mask_lanes2(i32 %active, i32 %index) #0 {
entry:
  %values = insertelement <2 x i32> zeroinitializer, i32 1, i32 %active
  %cmp = icmp ne <2 x i32> %values, zeroinitializer
  %bits = bitcast <2 x i1> %cmp to i2
  %wide = zext i2 %bits to i32
  %lane = extractelement <2 x i1> %cmp, i32 %index
  %selected = zext i1 %lane to i32
  %high = shl i32 %selected, 16
  %result = or i32 %wide, %high
  ret i32 %result
}

define i32 @mask_lanes4(i32 %active, i32 %index) #0 {
entry:
  %values = insertelement <4 x i32> zeroinitializer, i32 1, i32 %active
  %cmp = icmp ne <4 x i32> %values, zeroinitializer
  %bits = bitcast <4 x i1> %cmp to i4
  %wide = zext i4 %bits to i32
  %lane = extractelement <4 x i1> %cmp, i32 %index
  %selected = zext i1 %lane to i32
  %high = shl i32 %selected, 16
  %result = or i32 %wide, %high
  ret i32 %result
}

define i32 @mask_lanes16(i32 %active, i32 %index) #0 {
entry:
  %values = insertelement <16 x i32> zeroinitializer, i32 1, i32 %active
  %cmp = icmp ne <16 x i32> %values, zeroinitializer
  %bits = bitcast <16 x i1> %cmp to i16
  %wide = zext i16 %bits to i32
  %lane = extractelement <16 x i1> %cmp, i32 %index
  %selected = zext i1 %lane to i32
  %high = shl i32 %selected, 16
  %result = or i32 %wide, %high
  ret i32 %result
}

define i32 @mask_eq16(i32 %a, i32 %b) #0 {
entry:
  %av = insertelement <16 x i32> zeroinitializer, i32 %a, i32 0
  %bv = insertelement <16 x i32> zeroinitializer, i32 %b, i32 0
  %cmp = icmp eq <16 x i32> %av, %bv
  %bits = bitcast <16 x i1> %cmp to i16
  %wide = zext i16 %bits to i32
  ret i32 %wide
}

define i32 @mask_extract16(i32 %a, i32 %b, i32 %index) #0 {
entry:
  %av = insertelement <16 x i32> zeroinitializer, i32 %a, i32 0
  %bv = insertelement <16 x i32> zeroinitializer, i32 %b, i32 0
  %cmp = icmp eq <16 x i32> %av, %bv
  %lane = extractelement <16 x i1> %cmp, i32 %index
  %wide = zext i1 %lane to i32
  ret i32 %wide
}

define i64 @mask_zext2(i32 %a, i32 %b) #0 {
entry:
  %av = insertelement <2 x i32> zeroinitializer, i32 %a, i32 0
  %bv = insertelement <2 x i32> zeroinitializer, i32 %b, i32 0
  %cmp = icmp eq <2 x i32> %av, %bv
  %extended = zext <2 x i1> %cmp to <2 x i32>
  %bits = bitcast <2 x i32> %extended to i64
  ret i64 %bits
}

define i64 @mask_sext2(i32 %a, i32 %b) #0 {
entry:
  %av = insertelement <2 x i32> zeroinitializer, i32 %a, i32 0
  %bv = insertelement <2 x i32> zeroinitializer, i32 %b, i32 0
  %cmp = icmp eq <2 x i32> %av, %bv
  %extended = sext <2 x i1> %cmp to <2 x i32>
  %bits = bitcast <2 x i32> %extended to i64
  ret i64 %bits
}

define i32 @mask_store_load4(i32 %a, i32 %b) #0 {
entry:
  %av = insertelement <4 x i32> zeroinitializer, i32 %a, i32 0
  %bv = insertelement <4 x i32> <i32 0, i32 0, i32 1, i32 0>, i32 %b, i32 0
  %cmp = icmp eq <4 x i32> %av, %bv
  %bits = bitcast <4 x i1> %cmp to i4
  %byte = zext i4 %bits to i8
  %slot = alloca i8, align 1
  store volatile i8 %byte, i8* %slot, align 1
  %round = load volatile i8, i8* %slot, align 1
  %packed = trunc i8 %round to i4
  %wide = zext i4 %packed to i32
  ret i32 %wide
}

; Exercise both sides of the byte boundary independently (lanes 7 and 8).
define i32 @mask_store_load16(i32 %a, i32 %b) #0 {
entry:
  %av7 = insertelement <16 x i32> zeroinitializer, i32 %a, i32 7
  %av = insertelement <16 x i32> %av7, i32 %b, i32 8
  %cmp = icmp eq <16 x i32> %av, <i32 0, i32 0, i32 1, i32 0, i32 0, i32 1, i32 0, i32 5, i32 5, i32 1, i32 0, i32 1, i32 1, i32 0, i32 1, i32 0>
  %bits = bitcast <16 x i1> %cmp to i16
  %low = trunc i16 %bits to i8
  %shifted = lshr i16 %bits, 8
  %high = trunc i16 %shifted to i8
  %slot = alloca [2 x i8], align 1
  %loaddr = getelementptr [2 x i8], [2 x i8]* %slot, i32 0, i32 0
  %hiaddr = getelementptr [2 x i8], [2 x i8]* %slot, i32 0, i32 1
  store volatile i8 %low, i8* %loaddr, align 1
  store volatile i8 %high, i8* %hiaddr, align 1
  %roundlo = load volatile i8, i8* %loaddr, align 1
  %roundhi = load volatile i8, i8* %hiaddr, align 1
  %widehi = zext i8 %roundhi to i16
  %widelow = zext i8 %roundlo to i16
  %packedhi = shl i16 %widehi, 8
  %packed = or i16 %packedhi, %widelow
  %wide = zext i16 %packed to i32
  ret i32 %wide
}

; Protect the test instructions; a Function Attrs comment alone has no effect.
attributes #0 = { noinline optnone }
