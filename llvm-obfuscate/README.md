# STUDY LLVM PASS FOR OBFUSCATE CODE

LLVM PASS를 통해 코드 난독화를 구현하는 방식을 공부하는 자료입니다.

## Requirement
LLVM 15


## How to use

전체 빌드
$ make

빌드 결과 
$ ls -al *.so
-rwxr-xr-x 1 m m 2335256 Dec 13 13:16 Flatter.so
-rwxr-xr-x 1 m m 2041192 Dec 13 13:18 ObfuscatorCall.so
-rwxr-xr-x 1 m m 2121680 Dec 13 13:18 ObfuscatorString.so
-rwxr-xr-x 1 m m 1652384 Dec 13 13:18 Printer.so


### String Obfuscate
ref: https://github.com/tsarpaul/llvm-string-obfuscator.git

$ make obfstring


### Call
ref: https://die4taoam.tistory.com/65

$ make obfcall
결과 확인 
$ cat ObfuscatorCallTest.bc.bin.ll
다음과 같은 ir 을 확인 가능
```
; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %1 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  %2 = load i64, ptr @TTTTfez, align 8
  %3 = sub i64 %2, 1
  %tofncast = inttoptr i64 %3 to ptr
  call void %tofncast()
  call void @fez()
  ret i32 0
}
```
자세한 내용은 ref 참조

### Control-flow Flattening
ref: https://die4taoam.tistory.com/66

$ make flatting
결과 확인
$ cat Ex-ifstate.bc.bin.ll
```
; Function Attrs: noinline nounwind optnone uwtable
define dso_local i32 @main() #0 {
  %CASE = alloca i32, align 4
  %1 = alloca i32, align 4
  %2 = alloca ptr, align 8
  %3 = alloca i32, align 4
  %4 = alloca i32, align 4
  store i32 0, ptr %1, align 4
  store ptr @.str, ptr %2, align 8
  store i32 0, ptr %3, align 4
  store i32 0, ptr %4, align 4
  %5 = load i32, ptr %3, align 4
  store i32 8, ptr %CASE, align 4
  br label %6

6:                                                ; preds = %55, %0, %49, %48, %47, %46, %42, %35, %28, %21, %14, %10
  %7 = load i32, ptr %CASE, align 4
  switch i32 %7, label %55 [
    i32 8, label %8
    i32 10, label %10
    i32 11, label %11
    i32 14, label %14
    i32 18, label %18
    i32 21, label %21
    i32 25, label %25
    i32 28, label %28
    i32 32, label %32
    i32 35, label %35
    i32 39, label %39
    i32 42, label %42
    i32 46, label %46
    i32 47, label %47
    i32 48, label %48
    i32 49, label %49
    i32 50, label %50
  ]
```
자세한 내용은 ref 참조
