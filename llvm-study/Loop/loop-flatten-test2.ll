; RUN: opt < %s -S -loop-flatten -verify-loop-info -verify-dom-info -verify-scev -verify | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-v128:64:128-a:0:32-n32-S64"

; CHECK-LABEL: test2
; Same as above but non constant IV (which still cannot overflow)
define i32 @test2(i8 zeroext %I, i32 %val, i16* nocapture %A) {
entry:
  %conv = zext i8 %I to i32
  %cmp26 = icmp eq i8 %I, 0
  br i1 %cmp26, label %for.end13, label %for.body.lr.ph.split.us

for.body.lr.ph.split.us:                          ; preds = %entry
  br label %for.body.us
; CHECK: for.body.lr.ph.split.us:
; CHECK:   %flatten.tripcount = mul i32 %conv, %conv
; CHECK:   br label %for.body.us

for.body.us:                                      ; preds = %for.cond2.for.inc11_crit_edge.us, %for.body.lr.ph.split.us
  %i.027.us = phi i32 [ 0, %for.body.lr.ph.split.us ], [ %inc12.us, %for.cond2.for.inc11_crit_edge.us ]
  %mul.us = mul nuw nsw i32 %i.027.us, %conv
  br label %for.body6.us
; CHECK: for.body.us:
; CHECK:   %i.027.us = phi i32 [ 0, %for.body.lr.ph.split.us ], [ %inc12.us, %for.cond2.for.inc11_crit_edge.us ]
; CHECK:   %mul.us = mul nuw nsw i32 %i.027.us, %conv
; CHECK:   br label %for.body6.us

for.body6.us:                                     ; preds = %for.body.us, %for.body6.us
  %j.025.us = phi i32 [ 0, %for.body.us ], [ %inc.us, %for.body6.us ]
  %add.us = add nuw nsw i32 %j.025.us, %mul.us
  %arrayidx.us = getelementptr inbounds i16, i16* %A, i32 %add.us
  %0 = load i16, i16* %arrayidx.us, align 2
  %conv823.us = zext i16 %0 to i32
  %add9.us = add i32 %conv823.us, %val
  %conv10.us = trunc i32 %add9.us to i16
  store i16 %conv10.us, i16* %arrayidx.us, align 2
  %inc.us = add nuw nsw i32 %j.025.us, 1
  %exitcond = icmp ne i32 %inc.us, %conv
  br i1 %exitcond, label %for.body6.us, label %for.cond2.for.inc11_crit_edge.us
; CHECK: for.body6.us:
; CHECK:   %j.025.us = phi i32 [ 0, %for.body.us ]
; CHECK:   %add.us = add nuw nsw i32 %j.025.us, %mul.us
; CHECK:   %arrayidx.us = getelementptr inbounds i16, i16* %A, i32 %i.027.us
; CHECK:   %0 = load i16, i16* %arrayidx.us, align 2
; CHECK:   %conv823.us = zext i16 %0 to i32
; CHECK:   %add9.us = add i32 %conv823.us, %val
; CHECK:   %conv10.us = trunc i32 %add9.us to i16
; CHECK:   store i16 %conv10.us, i16* %arrayidx.us, align 2
; CHECK:   %inc.us = add nuw nsw i32 %j.025.us, 1
; CHECK:   %exitcond = icmp ne i32 %inc.us, %conv
; CHECK:   br label %for.cond2.for.inc11_crit_edge.us

for.cond2.for.inc11_crit_edge.us:                 ; preds = %for.body6.us
  %inc12.us = add nuw nsw i32 %i.027.us, 1
  %exitcond28 = icmp ne i32 %inc12.us, %conv
  br i1 %exitcond28, label %for.body.us, label %for.end13.loopexit
; CHECK: for.cond2.for.inc11_crit_edge.us:                 ; preds = %for.body6.us
; CHECK:   %inc12.us = add nuw nsw i32 %i.027.us, 1
; CHECK:   %exitcond28 = icmp ne i32 %inc12.us, %flatten.tripcount
; CHECK:   br i1 %exitcond28, label %for.body.us, label %for.end13.loopexit

for.end13.loopexit:                               ; preds = %for.cond2.for.inc11_crit_edge.us
  br label %for.end13

for.end13:                                        ; preds = %for.end13.loopexit, %entry
  %i.0.lcssa = phi i32 [ 0, %entry ], [ %conv, %for.end13.loopexit ]
  ret i32 %i.0.lcssa
}