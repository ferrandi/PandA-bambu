; ModuleID = 'gramschmidt.O1.bc'
source_filename = "LLVMDialectModule"

; Function Attrs: nofree nounwind
define void @kernel_gramschmidt(double* nocapture %0, double* nocapture %1, double* nocapture %2) local_unnamed_addr #0 !dbg !3 {
  br label %.preheader7, !dbg !7

.loopexit:                                        ; preds = %.lr.ph, %.preheader7
  %4 = icmp ult i64 %5, 5, !dbg !9
  br i1 %4, label %.preheader7, label %121, !dbg !7

.preheader7:                                      ; preds = %3, %.loopexit
  %5 = phi i64 [ 0, %3 ], [ %48, %.loopexit ]
  %6 = getelementptr double, double* %0, i64 %5, !dbg !10
  %7 = load double, double* %6, align 8, !dbg !11
  %8 = fmul double %7, %7, !dbg !12
  %9 = fadd double %8, 0.000000e+00, !dbg !13
  %10 = add nuw nsw i64 %5, 6, !dbg !14
  %11 = getelementptr double, double* %0, i64 %10, !dbg !10
  %12 = load double, double* %11, align 8, !dbg !11
  %13 = fmul double %12, %12, !dbg !12
  %14 = fadd double %9, %13, !dbg !13
  %15 = add nuw nsw i64 %5, 12, !dbg !14
  %16 = getelementptr double, double* %0, i64 %15, !dbg !10
  %17 = load double, double* %16, align 8, !dbg !11
  %18 = fmul double %17, %17, !dbg !12
  %19 = fadd double %14, %18, !dbg !13
  %20 = add nuw nsw i64 %5, 18, !dbg !14
  %21 = getelementptr double, double* %0, i64 %20, !dbg !10
  %22 = load double, double* %21, align 8, !dbg !11
  %23 = fmul double %22, %22, !dbg !12
  %24 = fadd double %19, %23, !dbg !13
  %25 = call double @llvm.sqrt.f64(double %24), !dbg !15
  %26 = mul nuw nsw i64 %5, 7, !dbg !16
  %27 = getelementptr double, double* %1, i64 %26, !dbg !17
  store double %25, double* %27, align 8, !dbg !18
  %28 = getelementptr double, double* %0, i64 %5, !dbg !19
  %29 = load double, double* %28, align 8, !dbg !20
  %30 = fdiv double %29, %25, !dbg !21
  %31 = getelementptr double, double* %2, i64 %5, !dbg !22
  store double %30, double* %31, align 8, !dbg !23
  %32 = add nuw nsw i64 %5, 6, !dbg !24
  %33 = getelementptr double, double* %0, i64 %32, !dbg !19
  %34 = load double, double* %33, align 8, !dbg !20
  %35 = fdiv double %34, %25, !dbg !21
  %36 = getelementptr double, double* %2, i64 %32, !dbg !22
  store double %35, double* %36, align 8, !dbg !23
  %37 = add nuw nsw i64 %5, 12, !dbg !24
  %38 = getelementptr double, double* %0, i64 %37, !dbg !19
  %39 = load double, double* %38, align 8, !dbg !20
  %40 = fdiv double %39, %25, !dbg !21
  %41 = getelementptr double, double* %2, i64 %37, !dbg !22
  store double %40, double* %41, align 8, !dbg !23
  %42 = add nuw nsw i64 %5, 18, !dbg !24
  %43 = getelementptr double, double* %0, i64 %42, !dbg !19
  %44 = load double, double* %43, align 8, !dbg !20
  %45 = fdiv double %44, %25, !dbg !21
  %46 = getelementptr double, double* %2, i64 %42, !dbg !22
  store double %45, double* %46, align 8, !dbg !23
  %47 = mul nuw nsw i64 %5, 6, !dbg !25
  %48 = add nuw nsw i64 %5, 1, !dbg !26
  %49 = icmp ult i64 %5, 5, !dbg !27
  br i1 %49, label %.lr.ph.preheader, label %.loopexit, !dbg !28

.lr.ph.preheader:                                 ; preds = %.preheader7
  %50 = getelementptr double, double* %2, i64 %5, !dbg !29
  %51 = add nuw nsw i64 %5, 6, !dbg !29
  %52 = getelementptr double, double* %2, i64 %51, !dbg !29
  %53 = add nuw nsw i64 %5, 12, !dbg !29
  %54 = getelementptr double, double* %2, i64 %53, !dbg !29
  %55 = add nuw nsw i64 %5, 18, !dbg !29
  %56 = getelementptr double, double* %2, i64 %55, !dbg !29
  %57 = getelementptr double, double* %2, i64 %5, !dbg !29
  %58 = add nuw nsw i64 %5, 6, !dbg !29
  %59 = getelementptr double, double* %2, i64 %58, !dbg !29
  %60 = add nuw nsw i64 %5, 12, !dbg !29
  %61 = getelementptr double, double* %2, i64 %60, !dbg !29
  %62 = add nuw nsw i64 %5, 18, !dbg !29
  %63 = getelementptr double, double* %2, i64 %62, !dbg !29
  br label %.lr.ph, !dbg !28

.lr.ph:                                           ; preds = %.lr.ph.preheader, %.lr.ph
  %64 = phi i64 [ %119, %.lr.ph ], [ %48, %.lr.ph.preheader ]
  %65 = add nuw nsw i64 %64, %47, !dbg !30
  %66 = getelementptr double, double* %1, i64 %65, !dbg !31
  store double 0.000000e+00, double* %66, align 8, !dbg !32
  %67 = load double, double* %50, align 8, !dbg !33
  %68 = getelementptr double, double* %0, i64 %64, !dbg !34
  %69 = load double, double* %68, align 8, !dbg !35
  %70 = fmul double %67, %69, !dbg !36
  %71 = fadd double %70, 0.000000e+00, !dbg !37
  store double %71, double* %66, align 8, !dbg !38
  %72 = load double, double* %52, align 8, !dbg !33
  %73 = add nuw nsw i64 %64, 6, !dbg !39
  %74 = getelementptr double, double* %0, i64 %73, !dbg !34
  %75 = load double, double* %74, align 8, !dbg !35
  %76 = fmul double %72, %75, !dbg !36
  %77 = load double, double* %66, align 8, !dbg !40
  %78 = fadd double %77, %76, !dbg !37
  store double %78, double* %66, align 8, !dbg !38
  %79 = load double, double* %54, align 8, !dbg !33
  %80 = add nuw nsw i64 %64, 12, !dbg !39
  %81 = getelementptr double, double* %0, i64 %80, !dbg !34
  %82 = load double, double* %81, align 8, !dbg !35
  %83 = fmul double %79, %82, !dbg !36
  %84 = load double, double* %66, align 8, !dbg !40
  %85 = fadd double %84, %83, !dbg !37
  store double %85, double* %66, align 8, !dbg !38
  %86 = load double, double* %56, align 8, !dbg !33
  %87 = add nuw nsw i64 %64, 18, !dbg !39
  %88 = getelementptr double, double* %0, i64 %87, !dbg !34
  %89 = load double, double* %88, align 8, !dbg !35
  %90 = fmul double %86, %89, !dbg !36
  %91 = load double, double* %66, align 8, !dbg !40
  %92 = fadd double %91, %90, !dbg !37
  store double %92, double* %66, align 8, !dbg !38
  %93 = getelementptr double, double* %0, i64 %64, !dbg !41
  %94 = load double, double* %93, align 8, !dbg !42
  %95 = load double, double* %57, align 8, !dbg !43
  %96 = fmul double %95, %92, !dbg !44
  %97 = fsub double %94, %96, !dbg !45
  store double %97, double* %93, align 8, !dbg !46
  %98 = add nuw nsw i64 %64, 6, !dbg !47
  %99 = getelementptr double, double* %0, i64 %98, !dbg !41
  %100 = load double, double* %99, align 8, !dbg !42
  %101 = load double, double* %59, align 8, !dbg !43
  %102 = load double, double* %66, align 8, !dbg !48
  %103 = fmul double %101, %102, !dbg !44
  %104 = fsub double %100, %103, !dbg !45
  store double %104, double* %99, align 8, !dbg !46
  %105 = add nuw nsw i64 %64, 12, !dbg !47
  %106 = getelementptr double, double* %0, i64 %105, !dbg !41
  %107 = load double, double* %106, align 8, !dbg !42
  %108 = load double, double* %61, align 8, !dbg !43
  %109 = load double, double* %66, align 8, !dbg !48
  %110 = fmul double %108, %109, !dbg !44
  %111 = fsub double %107, %110, !dbg !45
  store double %111, double* %106, align 8, !dbg !46
  %112 = add nuw nsw i64 %64, 18, !dbg !47
  %113 = getelementptr double, double* %0, i64 %112, !dbg !41
  %114 = load double, double* %113, align 8, !dbg !42
  %115 = load double, double* %63, align 8, !dbg !43
  %116 = load double, double* %66, align 8, !dbg !48
  %117 = fmul double %115, %116, !dbg !44
  %118 = fsub double %114, %117, !dbg !45
  store double %118, double* %113, align 8, !dbg !46
  %119 = add nuw nsw i64 %64, 1, !dbg !49
  %120 = icmp ult i64 %64, 5, !dbg !27
  br i1 %120, label %.lr.ph, label %.loopexit, !dbg !28

121:                                              ; preds = %.loopexit
  ret void, !dbg !50
}

; Function Attrs: nounwind readnone speculatable willreturn
declare double @llvm.sqrt.f64(double) #1

attributes #0 = { nofree nounwind }
attributes #1 = { nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "mlir", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "LLVMDialectModule", directory: "/")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = distinct !DISubprogram(name: "kernel_gramschmidt", linkageName: "kernel_gramschmidt", scope: null, file: !4, line: 2, type: !5, scopeLine: 2, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !6)
!4 = !DIFile(filename: "gramschmidt.llvm.mlir", directory: "/data1/home/curzel")
!5 = !DISubroutineType(types: !6)
!6 = !{}
!7 = !DILocation(line: 59, column: 5, scope: !8)
!8 = !DILexicalBlockFile(scope: !3, file: !4, discriminator: 0)
!9 = !DILocation(line: 58, column: 11, scope: !8)
!10 = !DILocation(line: 74, column: 11, scope: !8)
!11 = !DILocation(line: 75, column: 11, scope: !8)
!12 = !DILocation(line: 76, column: 11, scope: !8)
!13 = !DILocation(line: 78, column: 11, scope: !8)
!14 = !DILocation(line: 73, column: 11, scope: !8)
!15 = !DILocation(line: 84, column: 11, scope: !8)
!16 = !DILocation(line: 88, column: 11, scope: !8)
!17 = !DILocation(line: 89, column: 11, scope: !8)
!18 = !DILocation(line: 90, column: 5, scope: !8)
!19 = !DILocation(line: 103, column: 11, scope: !8)
!20 = !DILocation(line: 104, column: 11, scope: !8)
!21 = !DILocation(line: 105, column: 11, scope: !8)
!22 = !DILocation(line: 110, column: 11, scope: !8)
!23 = !DILocation(line: 111, column: 5, scope: !8)
!24 = !DILocation(line: 102, column: 11, scope: !8)
!25 = !DILocation(line: 87, column: 11, scope: !8)
!26 = !DILocation(line: 116, column: 11, scope: !8)
!27 = !DILocation(line: 121, column: 12, scope: !8)
!28 = !DILocation(line: 122, column: 5, scope: !8)
!29 = !DILocation(line: 0, scope: !8)
!30 = !DILocation(line: 127, column: 12, scope: !8)
!31 = !DILocation(line: 128, column: 12, scope: !8)
!32 = !DILocation(line: 129, column: 5, scope: !8)
!33 = !DILocation(line: 143, column: 12, scope: !8)
!34 = !DILocation(line: 148, column: 12, scope: !8)
!35 = !DILocation(line: 149, column: 12, scope: !8)
!36 = !DILocation(line: 150, column: 12, scope: !8)
!37 = !DILocation(line: 157, column: 12, scope: !8)
!38 = !DILocation(line: 163, column: 5, scope: !8)
!39 = !DILocation(line: 147, column: 12, scope: !8)
!40 = !DILocation(line: 156, column: 12, scope: !8)
!41 = !DILocation(line: 179, column: 12, scope: !8)
!42 = !DILocation(line: 180, column: 12, scope: !8)
!43 = !DILocation(line: 186, column: 12, scope: !8)
!44 = !DILocation(line: 193, column: 12, scope: !8)
!45 = !DILocation(line: 194, column: 12, scope: !8)
!46 = !DILocation(line: 200, column: 5, scope: !8)
!47 = !DILocation(line: 178, column: 12, scope: !8)
!48 = !DILocation(line: 192, column: 12, scope: !8)
!49 = !DILocation(line: 204, column: 12, scope: !8)
!50 = !DILocation(line: 210, column: 5, scope: !8)
