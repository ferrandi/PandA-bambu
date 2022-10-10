#le_10 = affine_set<(i): (10 - i >= 0)>
#ge_10 = affine_set<(i): (i - 10 >= 0)>
#eq_0_le_10 = affine_set<(i, j): (i == 0, 10 - j >= 0)>
#eq_10 = affine_set<(i): (i - 10 == 0)>

module {

    func @kernel(%S: memref<11x11xf64>{ llvm.name = "S" }, %D: memref<11x11x11xf64>{ llvm.name = "D" }, %u: memref<11x11x11xf64>{ llvm.name = "u" }, %v: memref<11x11x11xf64>{ llvm.name = "v" }, %t: memref<11x11x11xf64>{ llvm.name = "t" }, %r: memref<11x11x11xf64>{ llvm.name = "r" }, %t0:  memref<11x11x11xf64>{ llvm.name = "t0" }, %t1: memref<11x11x11xf64>{ llvm.name = "t1" }, %t2: memref<11x11x11xf64>{ llvm.name = "t2" }, %t3: memref<11x11x11xf64>{ llvm.name = "t3" }) {
        %zero = std.constant 0.0 : f64

        affine.for %c1 = 0 to 10 {
            affine.for %c2 = 0 to 10 {
                affine.for %c3 = 0 to 10 {
                    affine.store %zero, %t1[%c1,%c2,%c3] : memref<11x11x11xf64>
                    affine.for %c4 = 0 to 20 {
                        affine.if #le_10(%c4) {
                            %1 = affine.load %S[%c1,%c4] : memref<11x11xf64>
                            %2 = affine.load %u[%c2,%c3,%c4] : memref<11x11x11xf64>
                            %3 = mulf %1, %2 : f64
                            %4 = affine.load %t1[%c1,%c2,%c3] : memref<11x11x11xf64>
                            %5 = addf %3, %4 : f64
                            affine.store %5, %t1[%c1,%c2,%c3] : memref<11x11x11xf64>
                        }
                        affine.if #ge_10(%c4) {
                            %1 = affine.load %S[%c3,%c4 - 10] : memref<11x11xf64>
                            %2 = affine.load %t1[%c1,%c2,%c3] : memref<11x11x11xf64>
                            %3 = mulf %1, %2 : f64
                            %4 = affine.load %t0[%c4 - 10,%c1,%c2] : memref<11x11x11xf64>
                            %5 = addf %3, %4 : f64
                            affine.store %5, %t0[%c4 - 10,%c1,%c2] : memref<11x11x11xf64>
                        }
                        affine.if #eq_0_le_10(%c3,%c4) {
                            affine.store %zero, %t0[%c4,%c1,%c2] : memref<11x11x11xf64>
                        }
                    }
                }
            }
        }

        affine.for %c1 = 0 to 10 {
            affine.for %c2 = 0 to 10 {
                affine.for %c3 = 0 to 10 {
                    affine.store %zero, %t[%c1,%c2,%c3] : memref<11x11x11xf64>
                    affine.for %c4 = 0 to 20 {
                        affine.if #le_10(%c4) {
                            %1 = affine.load %S[%c1,%c4] : memref<11x11xf64>
                            %2 = affine.load %t0[%c2,%c3,%c4] : memref<11x11x11xf64>
                            %3 = mulf %1, %2 : f64
                            %4 = affine.load %t[%c1,%c2,%c3] : memref<11x11x11xf64>
                            %5 = addf %3, %4 : f64
                            affine.store %5, %t[%c1,%c2,%c3] : memref<11x11x11xf64>
                            affine.if #eq_10(%c4) {
                                %6 = affine.load %D[%c1,%c2,%c3]: memref<11x11x11xf64>
                                %7 = affine.load %t[%c1,%c2,%c3]: memref<11x11x11xf64>
                                %8 = mulf %6, %7 : f64
                                affine.store %8, %r[%c1,%c2,%c3] : memref<11x11x11xf64>
                            }
                        }
                        affine.if #ge_10(%c4) {
                            %1 = affine.load %S[%c3,%c4 - 10] : memref<11x11xf64>
                            %2 = affine.load %t[%c1,%c2,%c3] : memref<11x11x11xf64>
                            %3 = mulf %1, %2 : f64
                            %4 = affine.load %t3[%c4 - 10,%c1,%c2] : memref<11x11x11xf64>
                            %5 = addf %3, %4 : f64
                            affine.store %5, %t3[%c4 - 10,%c1,%c2] : memref<11x11x11xf64>
                        }
                        affine.if #eq_0_le_10(%c3,%c4) {
                            affine.store %zero, %t3[%c4,%c1,%c2] : memref<11x11x11xf64>
                        }
                    }
                }
            }
        }

        affine.for %c1 = 0 to 10 {
            affine.for %c2 = 0 to 10 {
                affine.for %c3 = 0 to 10 {
                    affine.for %c4 = 0 to 10 {
                        affine.if #eq_10(%c3) {
                            affine.store %zero, %v[%c4,%c1,%c2] : memref<11x11x11xf64>
                        }
                        affine.if #eq_10(%c4) {
                            affine.store %zero, %t2[%c1,%c2,%c3] : memref<11x11x11xf64>
                        }
                        %1 = affine.load %S[%c4,%c1] : memref<11x11xf64>
                        %2 = affine.load %t3[%c2,%c3,%c4] : memref<11x11x11xf64>
                        %3 = mulf %1, %2 : f64
                        %4 = affine.load %t2[%c1,%c2,%c3] : memref<11x11x11xf64>
                        %5 = addf %3, %4 : f64
                        affine.store %5,%t2[%c1,%c2,%c3] : memref<11x11x11xf64> 
                    }
                    affine.for %c4 = 10 to 20 {
                        %1 = affine.load %S[%c3,%c4 - 10] : memref<11x11xf64>
                        %2 = affine.load %t2[%c1,%c2,%c3] : memref<11x11x11xf64>
                        %3 = mulf %1, %2 : f64
                        %4 = affine.load %v[%c4 - 10,%c1,%c2] : memref<11x11x11xf64>
                        %5 = addf %3, %4 : f64
                        affine.store %5, %v[%c4 - 10,%c1,%c2] : memref<11x11x11xf64>
                    }
                }
            }
        }

        return
    }

}
