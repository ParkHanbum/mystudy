; RUN: opt < %s -S -loop-flatten -verify-loop-info -verify-dom-info -verify-scev -verify | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-v128:64:128-a:0:32-n32-S64"

; When the inner loop trip count is a constant and the step
; is 1, the InstCombine pass causes the transformation e.g.
; icmp ult i32 %inc, 20 -> icmp ult i32 %j, 19. This doesn't
; match the pattern (OuterPHI * InnerTripCount) + InnerPHI but
; we should still flatten the loop as the compare is removed
; later anyway.
define i32 @test9(i32* nocapture %A) {
entry:
  br label %for.cond1.preheader
; CHECK-LABEL: test9
; CHECK: entry:
; CHECK: %flatten.tripcount = mul i32 20, 11
; CHECK: br label %for.cond1.preheader

for.cond1.preheader:
  %i.017 = phi i32 [ 0, %entry ], [ %inc6, %for.cond.cleanup3 ]
  %mul = mul i32 %i.017, 20
  br label %for.body4
; CHECK: for.cond1.preheader:
; CHECK:   %i.017 = phi i32 [ 0, %entry ], [ %inc6, %for.cond.cleanup3 ]
; CHECK:   %mul = mul i32 %i.017, 20
; CHECK:   br label %for.body4

for.cond.cleanup3:
  %inc6 = add i32 %i.017, 1
  %cmp = icmp ult i32 %inc6, 11
  br i1 %cmp, label %for.cond1.preheader, label %for.cond.cleanup
; CHECK: for.cond.cleanup3:
; CHECK:   %inc6 = add i32 %i.017, 1
; CHECK:   %cmp = icmp ult i32 %inc6, %flatten.tripcount
; CHECK:   br i1 %cmp, label %for.cond1.preheader, label %for.cond.cleanup

for.body4:
  %j.016 = phi i32 [ 0, %for.cond1.preheader ], [ %inc, %for.body4 ]
  %add = add i32 %j.016, %mul
  %arrayidx = getelementptr inbounds i32, i32* %A, i32 %add
  store i32 30, i32* %arrayidx, align 4
  %inc = add nuw nsw i32 %j.016, 1
  %cmp2 = icmp ult i32 %j.016, 19
  br i1 %cmp2, label %for.body4, label %for.cond.cleanup3
; CHECK: for.body4
; CHECK:   %j.016 = phi i32 [ 0, %for.cond1.preheader ]
; CHECK:   %add = add i32 %j.016, %mul
; CHECK:   %arrayidx = getelementptr inbounds i32, i32* %A, i32 %i.017
; CHECK:   store i32 30, i32* %arrayidx, align 4
; CHECK:   %inc = add nuw nsw i32 %j.016, 1
; CHECK:   %cmp2 = icmp ult i32 %j.016, 19
; CHECK:   br label %for.cond.cleanup3

for.cond.cleanup:
  %0 = load i32, i32* %A, align 4
  ret i32 %0
}

declare i32 @func(i32)

