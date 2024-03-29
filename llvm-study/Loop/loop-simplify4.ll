; NOTE: Assertions have been autogenerated by utils/update_test_checks.py
; RUN: opt -S %s -passes=loop-instsimplify | FileCheck %s
; RUN: opt -S %s -passes='loop-mssa(loop-instsimplify)' -verify-memoryssa | FileCheck %s

; Test an inner loop that is only simplified when processing the outer loop, and
; an outer loop only simplified when processing the inner loop.
define i32 @test4(i32 %n, i32 %m, i32 %x) {
; CHECK-LABEL: @test4(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    br label [[LOOP:%.*]]
; CHECK:       loop:
; CHECK-NEXT:    [[I:%.*]] = phi i32 [ 0, [[ENTRY:%.*]] ], [ [[I_NEXT:%.*]], [[LOOP_LATCH:%.*]] ]
; CHECK-NEXT:    br label [[LOOP_INNER:%.*]]
; CHECK:       loop.inner:
; CHECK-NEXT:    [[J:%.*]] = phi i32 [ 0, [[LOOP]] ], [ [[J_NEXT:%.*]], [[LOOP_INNER]] ]
; CHECK-NEXT:    [[J_NEXT]] = add nsw i32 [[J]], 1
; CHECK-NEXT:    [[J_CMP:%.*]] = icmp slt i32 [[J_NEXT]], [[M:%.*]]
; CHECK-NEXT:    br i1 [[J_CMP]], label [[LOOP_INNER]], label [[LOOP_LATCH]]
; CHECK:       loop.latch:
; CHECK-NEXT:    [[I_NEXT]] = add nsw i32 [[I]], 1
; CHECK-NEXT:    [[I_CMP:%.*]] = icmp slt i32 [[I_NEXT]], [[N:%.*]]
; CHECK-NEXT:    br i1 [[I_CMP]], label [[LOOP]], label [[EXIT:%.*]]
; CHECK:       exit:
; CHECK-NEXT:    [[X_LCSSA:%.*]] = phi i32 [ [[X:%.*]], [[LOOP_LATCH]] ]
; CHECK-NEXT:    ret i32 [[X_LCSSA]]
;
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %i.next, %loop.latch ]
  %x.loop = phi i32 [ %x, %entry ], [ %x.inner.lcssa, %loop.latch ]
  %x.add = add nsw i32 %x.loop, 0
  br label %loop.inner

loop.inner:
  %j = phi i32 [ 0, %loop ], [ %j.next, %loop.inner ]
  %x.inner.loop = phi i32 [ %x.add, %loop ], [ %x.inner.add, %loop.inner ]
  %x.inner.add = add nsw i32 %x.inner.loop, 0
  %j.next = add nsw i32 %j, 1
  %j.cmp = icmp slt i32 %j.next, %m
  br i1 %j.cmp, label %loop.inner, label %loop.latch

loop.latch:
  %x.inner.lcssa = phi i32 [ %x.inner.loop, %loop.inner ]
  %i.next = add nsw i32 %i, 1
  %i.cmp = icmp slt i32 %i.next, %n
  br i1 %i.cmp, label %loop, label %exit

exit:
  %x.lcssa = phi i32 [ %x.loop, %loop.latch ]
  ret i32 %x.lcssa
}
