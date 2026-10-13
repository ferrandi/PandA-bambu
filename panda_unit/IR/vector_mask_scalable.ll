; Negative plugin fixture for LLVM 11 and later. Bambu must reject this type
; explicitly in the plugin; it must not serialize a fixed nelements count.
define i32 @mask_scalable(<vscale x 2 x i32> %input) #0 {
  %value = extractelement <vscale x 2 x i32> %input, i32 0
  ret i32 %value
}

; Protect the test instructions; a Function Attrs comment alone has no effect.
attributes #0 = { noinline optnone }
