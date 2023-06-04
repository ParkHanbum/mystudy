; ModuleID = 'GEP-study.bc'
source_filename = "GEP-study.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

%struct.ST = type { i32, double, %struct.RT }
%struct.RT = type { i8, [10 x [20 x i32]], i8 }

; Function Attrs: noinline nounwind optnone uwtable
define dso_local ptr @study1(ptr noundef %s) #0 {
entry:
  %s.addr = alloca ptr, align 8
  store ptr %s, ptr %s.addr, align 8
  %0 = load ptr, ptr %s.addr, align 8
  %arrayidx = getelementptr inbounds %struct.ST, ptr %0, i64 1
  %Z = getelementptr inbounds %struct.ST, ptr %arrayidx, i32 0, i32 2
  %B = getelementptr inbounds %struct.RT, ptr %Z, i32 0, i32 1
  %arrayidx1 = getelementptr inbounds [10 x [20 x i32]], ptr %B, i64 0, i64 5
  %arrayidx2 = getelementptr inbounds [20 x i32], ptr %arrayidx1, i64 0, i64 13
  %arrayidx3 = getelementptr inbounds [20 x i32], ptr %arrayidx1, i64 0, i64 14
  %res = icmp ne ptr %arrayidx2, %arrayidx3

  %a.off = getelementptr i8, ptr @A, i32 1
  %a.off1 = getelementptr i8, ptr @A, i32 -1
  %b.off = getelementptr i8, ptr @B, i32 1
  %b.off1 = getelementptr i8, ptr @B, i32 -1

  ret ptr %arrayidx2
}

@A = global i32 0
@B = global i32 0
@A.alias = alias i32, ptr @A

define i1 @globals_offset_inequal() {
  %a.off = getelementptr i8, ptr @A, i32 1
  %b.off = getelementptr i8, ptr @B, i32 1
  %res = icmp ne ptr %a.off, %b.off
  ret i1 %res
}

define i1 @accumalateConstantOffset() {
  %a.off = getelementptr i8, ptr @A, i32 1
  %a.off1 = getelementptr i8, ptr @A, i32 -1

  %b.off = getelementptr i8, ptr @B, i32 1
  %b.off1 = getelementptr i8, ptr @B, i32 -1

  %res = icmp ne ptr %a.off, %b.off
  ret i1 %res
}




attributes #0 = { noinline nounwind optnone uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 7, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 15.0.7 (https://github.com/llvm/llvm-project.git 8dfdcc7b7bf66834a761bd8de445840ef68e4d1a)"}
