; RUN: opt < %s -S -loop-flatten -verify-loop-info -verify-dom-info -verify-scev -verify | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-v128:64:128-a:0:32-n32-S64"

; CHECK-LABEL: test6
define i32 @test6(i8 zeroext %I, i16 zeroext %J) {
entry:
  %0 = lshr i8 %I, 1
  %div = zext i8 %0 to i32
  %cmp30 = icmp eq i8 %0, 0
  br i1 %cmp30, label %for.cond.cleanup, label %for.body.lr.ph

for.body.lr.ph:                                   ; preds = %entry
  %1 = lshr i16 %J, 1
  %div5 = zext i16 %1 to i32
  %cmp627 = icmp eq i16 %1, 0
  br i1 %cmp627, label %for.body.lr.ph.split, label %for.body.lr.ph.split.us

for.body.lr.ph.split.us:                          ; preds = %for.body.lr.ph
  br label %for.body.us
; CHECK: for.body.lr.ph.split.us:
; CHECK:   %flatten.tripcount = mul i32 %div5, %div
; CHECK:   br label %for.body.us

for.body.us:                                      ; preds = %for.cond3.for.cond.cleanup8_crit_edge.us, %for.body.lr.ph.split.us
  %i.032.us = phi i32 [ 0, %for.body.lr.ph.split.us ], [ %inc13.us, %for.cond3.for.cond.cleanup8_crit_edge.us ]
  %x.031.us = phi i32 [ 1, %for.body.lr.ph.split.us ], [ %xor.us.lcssa, %for.cond3.for.cond.cleanup8_crit_edge.us ]
  %mul.us = mul nuw nsw i32 %i.032.us, %div5
  br label %for.body9.us
; CHECK: for.body.us:
; CHECK:   %i.032.us = phi i32 [ 0, %for.body.lr.ph.split.us ], [ %inc13.us, %for.cond3.for.cond.cleanup8_crit_edge.us ]
; CHECK:   %x.031.us = phi i32 [ 1, %for.body.lr.ph.split.us ], [ %xor.us.lcssa, %for.cond3.for.cond.cleanup8_crit_edge.us ]
; CHECK:   %mul.us = mul nuw nsw i32 %i.032.us, %div5
; CHECK:   br label %for.body9.us

for.body9.us:                                     ; preds = %for.body.us, %for.body9.us
  %j.029.us = phi i32 [ 0, %for.body.us ], [ %inc.us, %for.body9.us ]
  %x.128.us = phi i32 [ %x.031.us, %for.body.us ], [ %xor.us, %for.body9.us ]
  %add.us = add nuw nsw i32 %j.029.us, %mul.us
  %call.us = tail call i32 @func(i32 %add.us)
  %sub.us = sub nsw i32 %call.us, %x.128.us
  %xor.us = xor i32 %sub.us, %x.128.us
  %inc.us = add nuw nsw i32 %j.029.us, 1
  %cmp6.us = icmp ult i32 %inc.us, %div5
  br i1 %cmp6.us, label %for.body9.us, label %for.cond3.for.cond.cleanup8_crit_edge.us
; CHECK: for.body9.us:
; CHECK:   %j.029.us = phi i32 [ 0, %for.body.us ]
; CHECK:   %x.128.us = phi i32 [ %x.031.us, %for.body.us ]
; CHECK:   %add.us = add nuw nsw i32 %j.029.us, %mul.us
; CHECK:   %call.us = tail call i32 @func(i32 %i.032.us)
; CHECK:   %sub.us = sub nsw i32 %call.us, %x.128.us
; CHECK:   %xor.us = xor i32 %sub.us, %x.128.us
; CHECK:   %inc.us = add nuw nsw i32 %j.029.us, 1
; CHECK:   %cmp6.us = icmp ult i32 %inc.us, %div5
; CHECK:   br label %for.cond3.for.cond.cleanup8_crit_edge.us

for.cond3.for.cond.cleanup8_crit_edge.us:         ; preds = %for.body9.us
  %xor.us.lcssa = phi i32 [ %xor.us, %for.body9.us ]
  %inc13.us = add nuw nsw i32 %i.032.us, 1
  %cmp.us = icmp ult i32 %inc13.us, %div
  br i1 %cmp.us, label %for.body.us, label %for.cond.cleanup.loopexit
; CHECK: for.cond3.for.cond.cleanup8_crit_edge.us:
; CHECK:   %xor.us.lcssa = phi i32 [ %xor.us, %for.body9.us ]
; CHECK:   %inc13.us = add nuw nsw i32 %i.032.us, 1
; CHECK:   %cmp.us = icmp ult i32 %inc13.us, %flatten.tripcount
; CHECK:   br i1 %cmp.us, label %for.body.us, label %for.cond.cleanup.loopexit

for.body.lr.ph.split:                             ; preds = %for.body.lr.ph
  br label %for.body

for.cond.cleanup.loopexit:                        ; preds = %for.cond3.for.cond.cleanup8_crit_edge.us
  %xor.us.lcssa.lcssa = phi i32 [ %xor.us.lcssa, %for.cond3.for.cond.cleanup8_crit_edge.us ]
  br label %for.cond.cleanup

for.cond.cleanup.loopexit34:                      ; preds = %for.body
  br label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.cond.cleanup.loopexit34, %for.cond.cleanup.loopexit, %entry
  %x.0.lcssa = phi i32 [ 1, %entry ], [ %xor.us.lcssa.lcssa, %for.cond.cleanup.loopexit ], [ 1, %for.cond.cleanup.loopexit34 ]
  ret i32 %x.0.lcssa

for.body:                                         ; preds = %for.body.lr.ph.split, %for.body
  %i.032 = phi i32 [ 0, %for.body.lr.ph.split ], [ %inc13, %for.body ]
  %inc13 = add nuw nsw i32 %i.032, 1
  %cmp = icmp ult i32 %inc13, %div
  br i1 %cmp, label %for.body, label %for.cond.cleanup.loopexit34
}