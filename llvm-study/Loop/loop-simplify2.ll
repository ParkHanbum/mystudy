; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -S %s -passes=loop-instsimplify | FileCheck %s
; RUN: opt -S %s -passes='loop-mssa(loop-instsimplify)' -verify-memoryssa | FileCheck %s

; Test basic loop structure that still has a simplification feed a prior PHI.
define i32 @test2(i32 %n, i32 %x) {
; CHECK-LABEL: @test2(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br label [[LOOP:%.*]]
; CHECK:       loop:
; CHECK-NEXT:    [[I:%.*]] = phi i32 [ 0, [[ENTRY:%.*]] ], [ [[I_NEXT:%.*]], [[LOOP]] ]
; CHECK-NEXT:    [[I_NEXT]] = add nsw i32 [[I]], 1
; CHECK-NEXT:    [[I_CMP:%.*]] = icmp slt i32 [[I_NEXT]], [[N:%.*]]
; CHECK-NEXT:    br i1 [[I_CMP]], label [[LOOP]], label [[EXIT:%.*]]
; CHECK:       exit:
; CHECK-NEXT:    [[X_LCSSA:%.*]] = phi i32 [ [[X:%.*]], [[LOOP]] ]
; CHECK-NEXT:    ret i32 [[X_LCSSA]]
;
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop ]
  %x.loop = phi i32 [ %x, %entry ], [ %x.next, %loop ]
  %x.next = add nsw i32 %x.loop, 0
  %i.next = add nsw i32 %i, 1
  %i.cmp = icmp slt i32 %i.next, %n
  br i1 %i.cmp, label %loop, label %exit

exit:
  %x.lcssa = phi i32 [ %x.loop, %loop ]
  ret i32 %x.lcssa
}
