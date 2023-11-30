; ModuleID = 'LLVMDialectModule'
source_filename = "LLVMDialectModule"

declare i8* @malloc(i64)

declare void @free(i8*)

define void @kernel_gramschmidt(double* %0, double* %1, double* %2) !dbg !3 {
  %4 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } undef, double* %0, 0, !dbg !7
  %5 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %4, double* %0, 1, !dbg !9
  %6 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %5, i64 0, 2, !dbg !10
  %7 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %6, i64 4, 3, 0, !dbg !11
  %8 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %7, i64 6, 4, 0, !dbg !12
  %9 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %8, i64 6, 3, 1, !dbg !13
  %10 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %9, i64 1, 4, 1, !dbg !14
  %11 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } undef, double* %1, 0, !dbg !15
  %12 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %11, double* %1, 1, !dbg !16
  %13 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %12, i64 0, 2, !dbg !17
  %14 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %13, i64 6, 3, 0, !dbg !18
  %15 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %14, i64 6, 4, 0, !dbg !19
  %16 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %15, i64 6, 3, 1, !dbg !20
  %17 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %16, i64 1, 4, 1, !dbg !21
  %18 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } undef, double* %2, 0, !dbg !22
  %19 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %18, double* %2, 1, !dbg !23
  %20 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %19, i64 0, 2, !dbg !24
  %21 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %20, i64 4, 3, 0, !dbg !25
  %22 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %21, i64 6, 4, 0, !dbg !26
  %23 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %22, i64 6, 3, 1, !dbg !27
  %24 = insertvalue { double*, double*, i64, [2 x i64], [2 x i64] } %23, i64 1, 4, 1, !dbg !28
  %25 = alloca double, i64 ptrtoint (double* getelementptr (double, double* null, i64 1) to i64), align 8, !dbg !29
  %26 = insertvalue { double*, double*, i64 } undef, double* %25, 0, !dbg !30
  %27 = insertvalue { double*, double*, i64 } %26, double* %25, 1, !dbg !31
  %28 = insertvalue { double*, double*, i64 } %27, i64 0, 2, !dbg !32
  br label %29, !dbg !33

29:                                               ; preds = %133, %3
  %30 = phi i64 [ %134, %133 ], [ 0, %3 ]
  %31 = icmp slt i64 %30, 6, !dbg !34
  br i1 %31, label %32, label %135, !dbg !35

32:                                               ; preds = %29
  store double 0.000000e+00, double* %25, align 8, !dbg !36
  br label %33, !dbg !37

33:                                               ; preds = %36, %32
  %34 = phi i64 [ %45, %36 ], [ 0, %32 ]
  %35 = icmp slt i64 %34, 4, !dbg !38
  br i1 %35, label %36, label %46, !dbg !39

36:                                               ; preds = %33
  %37 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %10, 1, !dbg !40
  %38 = mul i64 %34, 6, !dbg !41
  %39 = add i64 %38, %30, !dbg !42
  %40 = getelementptr double, double* %37, i64 %39, !dbg !43
  %41 = load double, double* %40, align 8, !dbg !44
  %42 = fmul double %41, %41, !dbg !45
  %43 = load double, double* %25, align 8, !dbg !46
  %44 = fadd double %43, %42, !dbg !47
  store double %44, double* %25, align 8, !dbg !48
  %45 = add i64 %34, 1, !dbg !49
  br label %33, !dbg !50

46:                                               ; preds = %33
  %47 = load double, double* %25, align 8, !dbg !51
  %48 = call double @llvm.sqrt.f64(double %47), !dbg !52
  %49 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %17, 1, !dbg !53
  %50 = mul i64 %30, 6, !dbg !54
  %51 = add i64 %50, %30, !dbg !55
  %52 = getelementptr double, double* %49, i64 %51, !dbg !56
  store double %48, double* %52, align 8, !dbg !57
  br label %53, !dbg !58

53:                                               ; preds = %56, %46
  %54 = phi i64 [ %67, %56 ], [ 0, %46 ]
  %55 = icmp slt i64 %54, 4, !dbg !59
  br i1 %55, label %56, label %68, !dbg !60

56:                                               ; preds = %53
  %57 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %10, 1, !dbg !61
  %58 = mul i64 %54, 6, !dbg !62
  %59 = add i64 %58, %30, !dbg !63
  %60 = getelementptr double, double* %57, i64 %59, !dbg !64
  %61 = load double, double* %60, align 8, !dbg !65
  %62 = fdiv double %61, %48, !dbg !66
  %63 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %24, 1, !dbg !67
  %64 = mul i64 %54, 6, !dbg !68
  %65 = add i64 %64, %30, !dbg !69
  %66 = getelementptr double, double* %63, i64 %65, !dbg !70
  store double %62, double* %66, align 8, !dbg !71
  %67 = add i64 %54, 1, !dbg !72
  br label %53, !dbg !73

68:                                               ; preds = %53
  %69 = add i64 %30, 1, !dbg !74
  br label %70, !dbg !75

70:                                               ; preds = %131, %68
  %71 = phi i64 [ %132, %131 ], [ %69, %68 ]
  %72 = icmp slt i64 %71, 6, !dbg !76
  br i1 %72, label %73, label %133, !dbg !77

73:                                               ; preds = %70
  %74 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %17, 1, !dbg !78
  %75 = mul i64 %30, 6, !dbg !79
  %76 = add i64 %75, %71, !dbg !80
  %77 = getelementptr double, double* %74, i64 %76, !dbg !81
  store double 0.000000e+00, double* %77, align 8, !dbg !82
  br label %78, !dbg !83

78:                                               ; preds = %81, %73
  %79 = phi i64 [ %103, %81 ], [ 0, %73 ]
  %80 = icmp slt i64 %79, 4, !dbg !84
  br i1 %80, label %81, label %104, !dbg !85

81:                                               ; preds = %78
  %82 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %24, 1, !dbg !86
  %83 = mul i64 %79, 6, !dbg !87
  %84 = add i64 %83, %30, !dbg !88
  %85 = getelementptr double, double* %82, i64 %84, !dbg !89
  %86 = load double, double* %85, align 8, !dbg !90
  %87 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %10, 1, !dbg !91
  %88 = mul i64 %79, 6, !dbg !92
  %89 = add i64 %88, %71, !dbg !93
  %90 = getelementptr double, double* %87, i64 %89, !dbg !94
  %91 = load double, double* %90, align 8, !dbg !95
  %92 = fmul double %86, %91, !dbg !96
  %93 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %17, 1, !dbg !97
  %94 = mul i64 %30, 6, !dbg !98
  %95 = add i64 %94, %71, !dbg !99
  %96 = getelementptr double, double* %93, i64 %95, !dbg !100
  %97 = load double, double* %96, align 8, !dbg !101
  %98 = fadd double %97, %92, !dbg !102
  %99 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %17, 1, !dbg !103
  %100 = mul i64 %30, 6, !dbg !104
  %101 = add i64 %100, %71, !dbg !105
  %102 = getelementptr double, double* %99, i64 %101, !dbg !106
  store double %98, double* %102, align 8, !dbg !107
  %103 = add i64 %79, 1, !dbg !108
  br label %78, !dbg !109

104:                                              ; preds = %78
  br label %105, !dbg !110

105:                                              ; preds = %108, %104
  %106 = phi i64 [ %130, %108 ], [ 0, %104 ]
  %107 = icmp slt i64 %106, 4, !dbg !111
  br i1 %107, label %108, label %131, !dbg !112

108:                                              ; preds = %105
  %109 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %10, 1, !dbg !113
  %110 = mul i64 %106, 6, !dbg !114
  %111 = add i64 %110, %71, !dbg !115
  %112 = getelementptr double, double* %109, i64 %111, !dbg !116
  %113 = load double, double* %112, align 8, !dbg !117
  %114 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %24, 1, !dbg !118
  %115 = mul i64 %106, 6, !dbg !119
  %116 = add i64 %115, %30, !dbg !120
  %117 = getelementptr double, double* %114, i64 %116, !dbg !121
  %118 = load double, double* %117, align 8, !dbg !122
  %119 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %17, 1, !dbg !123
  %120 = mul i64 %30, 6, !dbg !124
  %121 = add i64 %120, %71, !dbg !125
  %122 = getelementptr double, double* %119, i64 %121, !dbg !126
  %123 = load double, double* %122, align 8, !dbg !127
  %124 = fmul double %118, %123, !dbg !128
  %125 = fsub double %113, %124, !dbg !129
  %126 = extractvalue { double*, double*, i64, [2 x i64], [2 x i64] } %10, 1, !dbg !130
  %127 = mul i64 %106, 6, !dbg !131
  %128 = add i64 %127, %71, !dbg !132
  %129 = getelementptr double, double* %126, i64 %128, !dbg !133
  store double %125, double* %129, align 8, !dbg !134
  %130 = add i64 %106, 1, !dbg !135
  br label %105, !dbg !136

131:                                              ; preds = %105
  %132 = add i64 %71, 1, !dbg !137
  br label %70, !dbg !138

133:                                              ; preds = %70
  %134 = add i64 %30, 1, !dbg !139
  br label %29, !dbg !140

135:                                              ; preds = %29
  ret void, !dbg !141
}

; Function Attrs: nofree nosync nounwind readnone speculatable willreturn
declare double @llvm.sqrt.f64(double) #0

attributes #0 = { nofree nosync nounwind readnone speculatable willreturn }

!llvm.dbg.cu = !{!0}
!llvm.module.flags = !{!2}

!0 = distinct !DICompileUnit(language: DW_LANG_C, file: !1, producer: "mlir", isOptimized: true, runtimeVersion: 0, emissionKind: FullDebug)
!1 = !DIFile(filename: "LLVMDialectModule", directory: "/")
!2 = !{i32 2, !"Debug Info Version", i32 3}
!3 = distinct !DISubprogram(name: "kernel_gramschmidt", linkageName: "kernel_gramschmidt", scope: null, file: !4, line: 2, type: !5, scopeLine: 2, spFlags: DISPFlagDefinition | DISPFlagOptimized, unit: !0, retainedNodes: !6)
!4 = !DIFile(filename: "gramschmidt.llvm.mlir", directory: "/data1/home/curzel")
!5 = !DISubroutineType(types: !6)
!6 = !{}
!7 = !DILocation(line: 4, column: 10, scope: !8)
!8 = !DILexicalBlockFile(scope: !3, file: !4, discriminator: 0)
!9 = !DILocation(line: 5, column: 10, scope: !8)
!10 = !DILocation(line: 7, column: 10, scope: !8)
!11 = !DILocation(line: 9, column: 10, scope: !8)
!12 = !DILocation(line: 11, column: 10, scope: !8)
!13 = !DILocation(line: 13, column: 11, scope: !8)
!14 = !DILocation(line: 15, column: 11, scope: !8)
!15 = !DILocation(line: 17, column: 11, scope: !8)
!16 = !DILocation(line: 18, column: 11, scope: !8)
!17 = !DILocation(line: 20, column: 11, scope: !8)
!18 = !DILocation(line: 22, column: 11, scope: !8)
!19 = !DILocation(line: 24, column: 11, scope: !8)
!20 = !DILocation(line: 26, column: 11, scope: !8)
!21 = !DILocation(line: 28, column: 11, scope: !8)
!22 = !DILocation(line: 30, column: 11, scope: !8)
!23 = !DILocation(line: 31, column: 11, scope: !8)
!24 = !DILocation(line: 33, column: 11, scope: !8)
!25 = !DILocation(line: 35, column: 11, scope: !8)
!26 = !DILocation(line: 37, column: 11, scope: !8)
!27 = !DILocation(line: 39, column: 11, scope: !8)
!28 = !DILocation(line: 41, column: 11, scope: !8)
!29 = !DILocation(line: 47, column: 11, scope: !8)
!30 = !DILocation(line: 49, column: 11, scope: !8)
!31 = !DILocation(line: 50, column: 11, scope: !8)
!32 = !DILocation(line: 52, column: 11, scope: !8)
!33 = !DILocation(line: 56, column: 5, scope: !8)
!34 = !DILocation(line: 58, column: 11, scope: !8)
!35 = !DILocation(line: 59, column: 5, scope: !8)
!36 = !DILocation(line: 61, column: 5, scope: !8)
!37 = !DILocation(line: 65, column: 5, scope: !8)
!38 = !DILocation(line: 67, column: 11, scope: !8)
!39 = !DILocation(line: 68, column: 5, scope: !8)
!40 = !DILocation(line: 70, column: 11, scope: !8)
!41 = !DILocation(line: 72, column: 11, scope: !8)
!42 = !DILocation(line: 73, column: 11, scope: !8)
!43 = !DILocation(line: 74, column: 11, scope: !8)
!44 = !DILocation(line: 75, column: 11, scope: !8)
!45 = !DILocation(line: 76, column: 11, scope: !8)
!46 = !DILocation(line: 77, column: 11, scope: !8)
!47 = !DILocation(line: 78, column: 11, scope: !8)
!48 = !DILocation(line: 79, column: 5, scope: !8)
!49 = !DILocation(line: 80, column: 11, scope: !8)
!50 = !DILocation(line: 81, column: 5, scope: !8)
!51 = !DILocation(line: 83, column: 11, scope: !8)
!52 = !DILocation(line: 84, column: 11, scope: !8)
!53 = !DILocation(line: 85, column: 11, scope: !8)
!54 = !DILocation(line: 87, column: 11, scope: !8)
!55 = !DILocation(line: 88, column: 11, scope: !8)
!56 = !DILocation(line: 89, column: 11, scope: !8)
!57 = !DILocation(line: 90, column: 5, scope: !8)
!58 = !DILocation(line: 94, column: 5, scope: !8)
!59 = !DILocation(line: 96, column: 11, scope: !8)
!60 = !DILocation(line: 97, column: 5, scope: !8)
!61 = !DILocation(line: 99, column: 11, scope: !8)
!62 = !DILocation(line: 101, column: 11, scope: !8)
!63 = !DILocation(line: 102, column: 11, scope: !8)
!64 = !DILocation(line: 103, column: 11, scope: !8)
!65 = !DILocation(line: 104, column: 11, scope: !8)
!66 = !DILocation(line: 105, column: 11, scope: !8)
!67 = !DILocation(line: 106, column: 11, scope: !8)
!68 = !DILocation(line: 108, column: 11, scope: !8)
!69 = !DILocation(line: 109, column: 11, scope: !8)
!70 = !DILocation(line: 110, column: 11, scope: !8)
!71 = !DILocation(line: 111, column: 5, scope: !8)
!72 = !DILocation(line: 112, column: 11, scope: !8)
!73 = !DILocation(line: 113, column: 5, scope: !8)
!74 = !DILocation(line: 116, column: 11, scope: !8)
!75 = !DILocation(line: 119, column: 5, scope: !8)
!76 = !DILocation(line: 121, column: 12, scope: !8)
!77 = !DILocation(line: 122, column: 5, scope: !8)
!78 = !DILocation(line: 124, column: 12, scope: !8)
!79 = !DILocation(line: 126, column: 12, scope: !8)
!80 = !DILocation(line: 127, column: 12, scope: !8)
!81 = !DILocation(line: 128, column: 12, scope: !8)
!82 = !DILocation(line: 129, column: 5, scope: !8)
!83 = !DILocation(line: 133, column: 5, scope: !8)
!84 = !DILocation(line: 135, column: 12, scope: !8)
!85 = !DILocation(line: 136, column: 5, scope: !8)
!86 = !DILocation(line: 138, column: 12, scope: !8)
!87 = !DILocation(line: 140, column: 12, scope: !8)
!88 = !DILocation(line: 141, column: 12, scope: !8)
!89 = !DILocation(line: 142, column: 12, scope: !8)
!90 = !DILocation(line: 143, column: 12, scope: !8)
!91 = !DILocation(line: 144, column: 12, scope: !8)
!92 = !DILocation(line: 146, column: 12, scope: !8)
!93 = !DILocation(line: 147, column: 12, scope: !8)
!94 = !DILocation(line: 148, column: 12, scope: !8)
!95 = !DILocation(line: 149, column: 12, scope: !8)
!96 = !DILocation(line: 150, column: 12, scope: !8)
!97 = !DILocation(line: 151, column: 12, scope: !8)
!98 = !DILocation(line: 153, column: 12, scope: !8)
!99 = !DILocation(line: 154, column: 12, scope: !8)
!100 = !DILocation(line: 155, column: 12, scope: !8)
!101 = !DILocation(line: 156, column: 12, scope: !8)
!102 = !DILocation(line: 157, column: 12, scope: !8)
!103 = !DILocation(line: 158, column: 12, scope: !8)
!104 = !DILocation(line: 160, column: 12, scope: !8)
!105 = !DILocation(line: 161, column: 12, scope: !8)
!106 = !DILocation(line: 162, column: 12, scope: !8)
!107 = !DILocation(line: 163, column: 5, scope: !8)
!108 = !DILocation(line: 164, column: 12, scope: !8)
!109 = !DILocation(line: 165, column: 5, scope: !8)
!110 = !DILocation(line: 170, column: 5, scope: !8)
!111 = !DILocation(line: 172, column: 12, scope: !8)
!112 = !DILocation(line: 173, column: 5, scope: !8)
!113 = !DILocation(line: 175, column: 12, scope: !8)
!114 = !DILocation(line: 177, column: 12, scope: !8)
!115 = !DILocation(line: 178, column: 12, scope: !8)
!116 = !DILocation(line: 179, column: 12, scope: !8)
!117 = !DILocation(line: 180, column: 12, scope: !8)
!118 = !DILocation(line: 181, column: 12, scope: !8)
!119 = !DILocation(line: 183, column: 12, scope: !8)
!120 = !DILocation(line: 184, column: 12, scope: !8)
!121 = !DILocation(line: 185, column: 12, scope: !8)
!122 = !DILocation(line: 186, column: 12, scope: !8)
!123 = !DILocation(line: 187, column: 12, scope: !8)
!124 = !DILocation(line: 189, column: 12, scope: !8)
!125 = !DILocation(line: 190, column: 12, scope: !8)
!126 = !DILocation(line: 191, column: 12, scope: !8)
!127 = !DILocation(line: 192, column: 12, scope: !8)
!128 = !DILocation(line: 193, column: 12, scope: !8)
!129 = !DILocation(line: 194, column: 12, scope: !8)
!130 = !DILocation(line: 195, column: 12, scope: !8)
!131 = !DILocation(line: 197, column: 12, scope: !8)
!132 = !DILocation(line: 198, column: 12, scope: !8)
!133 = !DILocation(line: 199, column: 12, scope: !8)
!134 = !DILocation(line: 200, column: 5, scope: !8)
!135 = !DILocation(line: 201, column: 12, scope: !8)
!136 = !DILocation(line: 202, column: 5, scope: !8)
!137 = !DILocation(line: 204, column: 12, scope: !8)
!138 = !DILocation(line: 205, column: 5, scope: !8)
!139 = !DILocation(line: 207, column: 12, scope: !8)
!140 = !DILocation(line: 208, column: 5, scope: !8)
!141 = !DILocation(line: 210, column: 5, scope: !8)
