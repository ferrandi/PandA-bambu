; Vector with non-byte, non-i1 lanes: <4 x i7>. Exercises the logical lane count
; (nelements) and the characterization lane count derived from the element width
; (128 / 7 = 18 lanes), which the byte-padded allocation width used to hide.
target triple = "i386-pc-linux-gnu"

define i32 @vec_i7_add(i32 %a, i32 %b) #0 {
entry:
  %a0 = and i32 %a, 127
  %ta0 = trunc i32 %a0 to i7
  %va0 = insertelement <4 x i7> undef, i7 %ta0, i32 0
  %as1 = lshr i32 %a, 7
  %a1 = and i32 %as1, 127
  %ta1 = trunc i32 %a1 to i7
  %va1 = insertelement <4 x i7> %va0, i7 %ta1, i32 1
  %as2 = lshr i32 %a, 14
  %a2 = and i32 %as2, 127
  %ta2 = trunc i32 %a2 to i7
  %va2 = insertelement <4 x i7> %va1, i7 %ta2, i32 2
  %as3 = lshr i32 %a, 21
  %a3 = and i32 %as3, 127
  %ta3 = trunc i32 %a3 to i7
  %va3 = insertelement <4 x i7> %va2, i7 %ta3, i32 3
  %b0 = and i32 %b, 127
  %tb0 = trunc i32 %b0 to i7
  %vb0 = insertelement <4 x i7> undef, i7 %tb0, i32 0
  %bs1 = lshr i32 %b, 7
  %b1 = and i32 %bs1, 127
  %tb1 = trunc i32 %b1 to i7
  %vb1 = insertelement <4 x i7> %vb0, i7 %tb1, i32 1
  %bs2 = lshr i32 %b, 14
  %b2 = and i32 %bs2, 127
  %tb2 = trunc i32 %b2 to i7
  %vb2 = insertelement <4 x i7> %vb1, i7 %tb2, i32 2
  %bs3 = lshr i32 %b, 21
  %b3 = and i32 %bs3, 127
  %tb3 = trunc i32 %b3 to i7
  %vb3 = insertelement <4 x i7> %vb2, i7 %tb3, i32 3
  %sum = add <4 x i7> %va3, %vb3
  %e0 = extractelement <4 x i7> %sum, i32 0
  %z0 = zext i7 %e0 to i32
  %e1 = extractelement <4 x i7> %sum, i32 1
  %z1 = zext i7 %e1 to i32
  %s1 = shl i32 %z1, 7
  %p1 = or i32 %z0, %s1
  %e2 = extractelement <4 x i7> %sum, i32 2
  %z2 = zext i7 %e2 to i32
  %s2 = shl i32 %z2, 14
  %p2 = or i32 %p1, %s2
  %e3 = extractelement <4 x i7> %sum, i32 3
  %z3 = zext i7 %e3 to i32
  %s3 = shl i32 %z3, 21
  %p3 = or i32 %p2, %s3
  ret i32 %p3
}

; Protect the test instructions; a Function Attrs comment alone has no effect.
attributes #0 = { noinline optnone }
