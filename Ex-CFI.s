; ModuleID = 'Ex-CFI.c'
source_filename = "Ex-CFI.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-f80:128-n8:16:32:64-S128"
target triple = "x86_64-pc-linux-gnu"

%struct.auth = type { [4 x i8], {}* }

@.str = private unnamed_addr constant [28 x i8] c"Authenticated successfully\0A\00", align 1
@.str.1 = private unnamed_addr constant [23 x i8] c"Authentication failed\0A\00", align 1
@.str.2 = private unnamed_addr constant [5 x i8] c"pass\00", align 1
@.str.3 = private unnamed_addr constant [22 x i8] c"Enter your password:\0A\00", align 1
@.str.4 = private unnamed_addr constant [3 x i8] c"%s\00", align 1
@.src = private unnamed_addr constant [9 x i8] c"Ex-CFI.c\00", align 1
@anon.bbe611c5dcb19f4770bd073c5f77b02f.0 = private unnamed_addr constant { i16, i16, [23 x i8] } { i16 -1, i16 0, [23 x i8] c"'void (struct auth *)'\00" }

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @success() #0 !type !3 !type !4 {
  %1 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([28 x i8], [28 x i8]* @.str, i64 0, i64 0))
  ret void
}

declare !type !5 !type !6 dso_local i32 @printf(i8*, ...) #1

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @failure() #0 !type !3 !type !4 {
  %1 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([23 x i8], [23 x i8]* @.str.1, i64 0, i64 0))
  ret void
}

; Function Attrs: noinline nounwind optnone uwtable
define dso_local void @auth(%struct.auth* %0) #0 !type !7 !type !8 {
  %2 = alloca %struct.auth*, align 8
  store %struct.auth* %0, %struct.auth** %2, align 8
  %3 = load %struct.auth*, %struct.auth** %2, align 8
  %4 = getelementptr inbounds %struct.auth, %struct.auth* %3, i32 0, i32 0
  %5 = getelementptr inbounds [4 x i8], [4 x i8]* %4, i64 0, i64 0
  %6 = call i32 @strcmp(i8* %5, i8* getelementptr inbounds ([5 x i8], [5 x i8]* @.str.2, i64 0, i64 0)) #5
  %7 = icmp eq i32 %6, 0
  br i1 %7, label %8, label %12

8:                                                ; preds = %1
  %9 = load %struct.auth*, %struct.auth** %2, align 8
  %10 = getelementptr inbounds %struct.auth, %struct.auth* %9, i32 0, i32 1
  %11 = bitcast {}** %10 to void (%struct.auth*)**
  store void (%struct.auth*)* bitcast (void ()* @success to void (%struct.auth*)*), void (%struct.auth*)** %11, align 8
  br label %16

12:                                               ; preds = %1
  %13 = load %struct.auth*, %struct.auth** %2, align 8
  %14 = getelementptr inbounds %struct.auth, %struct.auth* %13, i32 0, i32 1
  %15 = bitcast {}** %14 to void (%struct.auth*)**
  store void (%struct.auth*)* bitcast (void ()* @failure to void (%struct.auth*)*), void (%struct.auth*)** %15, align 8
  br label %16

16:                                               ; preds = %12, %8
  ret void
}

; Function Attrs: nounwind readonly
declare !type !9 !type !10 dso_local i32 @strcmp(i8*, i8*) #2

; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main(i32 %0, i8** %1) #0 !type !11 !type !12 {
  %3 = alloca i32, align 4
  %4 = alloca i8**, align 8
  %5 = alloca %struct.auth, align 8
  store i32 %0, i32* %3, align 4
  store i8** %1, i8*** %4, align 8
  %6 = getelementptr inbounds %struct.auth, %struct.auth* %5, i32 0, i32 1
  %7 = bitcast {}** %6 to void (%struct.auth*)**
  store void (%struct.auth*)* @auth, void (%struct.auth*)** %7, align 8
  %8 = call i32 (i8*, ...) @printf(i8* getelementptr inbounds ([22 x i8], [22 x i8]* @.str.3, i64 0, i64 0))
  %9 = getelementptr inbounds %struct.auth, %struct.auth* %5, i32 0, i32 0
  %10 = call i32 (i8*, ...) @__isoc99_scanf(i8* getelementptr inbounds ([3 x i8], [3 x i8]* @.str.4, i64 0, i64 0), [4 x i8]* %9)
  %11 = getelementptr inbounds %struct.auth, %struct.auth* %5, i32 0, i32 1
  %12 = bitcast {}** %11 to void (%struct.auth*)**
  %13 = load void (%struct.auth*)*, void (%struct.auth*)** %12, align 8
  %14 = bitcast void (%struct.auth*)* %13 to i8*, !nosanitize !13
  %15 = call i1 @llvm.type.test(i8* %14, metadata !"_ZTSFvP4authE"), !nosanitize !13
  br i1 %15, label %17, label %16, !nosanitize !13

16:                                               ; preds = %2
  call void @llvm.trap() #6, !nosanitize !13
  unreachable, !nosanitize !13

17:                                               ; preds = %2
  call void %13(%struct.auth* %5)
  ret i32 0
}

declare !type !5 !type !6 dso_local i32 @__isoc99_scanf(i8*, ...) #1

; Function Attrs: nounwind readnone willreturn
declare i1 @llvm.type.test(i8*, metadata) #3

; Function Attrs: cold noreturn nounwind
declare void @llvm.trap() #4

attributes #0 = { noinline nounwind optnone uwtable "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "min-legal-vector-width"="0" "no-infs-fp-math"="false" "no-jump-tables"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #1 = { "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #2 = { nounwind readonly "correctly-rounded-divide-sqrt-fp-math"="false" "disable-tail-calls"="false" "frame-pointer"="all" "less-precise-fpmad"="false" "no-infs-fp-math"="false" "no-nans-fp-math"="false" "no-signed-zeros-fp-math"="false" "no-trapping-math"="false" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "unsafe-fp-math"="false" "use-soft-float"="false" }
attributes #3 = { nounwind readnone willreturn }
attributes #4 = { cold noreturn nounwind }
attributes #5 = { nounwind readonly }
attributes #6 = { noreturn nounwind }

!llvm.module.flags = !{!0, !1}
!llvm.ident = !{!2}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 4, !"CFI Canonical Jump Tables", i32 1}
!2 = !{!"clang version 10.0.0-4ubuntu1 "}
!3 = !{i64 0, !"_ZTSFvE"}
!4 = !{i64 0, !"_ZTSFvE.generalized"}
!5 = !{i64 0, !"_ZTSFiPKczE"}
!6 = !{i64 0, !"_ZTSFiPKvzE.generalized"}
!7 = !{i64 0, !"_ZTSFvP4authE"}
!8 = !{i64 0, !"_ZTSFvPvE.generalized"}
!9 = !{i64 0, !"_ZTSFiPKcS0_E"}
!10 = !{i64 0, !"_ZTSFiPKvS0_E.generalized"}
!11 = !{i64 0, !"_ZTSFiiPPcE"}
!12 = !{i64 0, !"_ZTSFiiPvE.generalized"}
!13 = !{}
