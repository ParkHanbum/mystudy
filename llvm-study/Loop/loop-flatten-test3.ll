; RUN: opt < %s -S -loop-flatten -verify-loop-info -verify-dom-info -verify-scev -verify | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-v128:64:128-a:0:32-n32-S64"


; CHECK-LABEL: test3
; Same as above, uses load to determine it can't overflow
define i32 @test3(i32 %N, i32 %val, i16* nocapture %A) local_unnamed_addr #0 {
entry:
  %cmp21 = icmp eq i32 %N, 0
  br i1 %cmp21, label %for.end8, label %for.body.lr.ph.split.us

for.body.lr.ph.split.us:                          ; preds = %entry
  br label %for.body.us
; CHECK: for.body.lr.ph.split.us:
; CHECK:   %flatten.tripcount = mul i32 %N, %N
; CHECK:   br label %for.body.us

for.body.us:                                      ; preds = %for.cond1.for.inc6_crit_edge.us, %for.body.lr.ph.split.us
  %i.022.us = phi i32 [ 0, %for.body.lr.ph.split.us ], [ %inc7.us, %for.cond1.for.inc6_crit_edge.us ]
  %mul.us = mul i32 %i.022.us, %N
  br label %for.body3.us
; CHECK: for.body.us:
; CHECK:   %i.022.us = phi i32 [ 0, %for.body.lr.ph.split.us ], [ %inc7.us, %for.cond1.for.inc6_crit_edge.us ]
; CHECK:   %mul.us = mul i32 %i.022.us, %N
; CHECK:   br label %for.body3.us

for.body3.us:                                     ; preds = %for.body.us, %for.body3.us
  %j.020.us = phi i32 [ 0, %for.body.us ], [ %inc.us, %for.body3.us ]
  %add.us = add i32 %j.020.us, %mul.us
  %arrayidx.us = getelementptr inbounds i16, i16* %A, i32 %add.us
  %0 = load i16, i16* %arrayidx.us, align 2
  %conv18.us = zext i16 %0 to i32
  %add4.us = add i32 %conv18.us, %val
  %conv5.us = trunc i32 %add4.us to i16
  store i16 %conv5.us, i16* %arrayidx.us, align 2
  %inc.us = add nuw i32 %j.020.us, 1
  %exitcond = icmp ne i32 %inc.us, %N
  br i1 %exitcond, label %for.body3.us, label %for.cond1.for.inc6_crit_edge.us
; CHECK: for.body3.us:
; CHECK:   %j.020.us = phi i32 [ 0, %for.body.us ]
; CHECK:   %add.us = add i32 %j.020.us, %mul.us
; CHECK:   %arrayidx.us = getelementptr inbounds i16, i16* %A, i32 %i.022.us
; CHECK:   %0 = load i16, i16* %arrayidx.us, align 2
; CHECK:   %conv18.us = zext i16 %0 to i32
; CHECK:   %add4.us = add i32 %conv18.us, %val
; CHECK:   %conv5.us = trunc i32 %add4.us to i16
; CHECK:   store i16 %conv5.us, i16* %arrayidx.us, align 2
; CHECK:   %inc.us = add nuw i32 %j.020.us, 1
; CHECK:   %exitcond = icmp ne i32 %inc.us, %N
; CHECK:   br label %for.cond1.for.inc6_crit_edge.us

for.cond1.for.inc6_crit_edge.us:                  ; preds = %for.body3.us
  %inc7.us = add nuw i32 %i.022.us, 1
  %exitcond23 = icmp ne i32 %inc7.us, %N
  br i1 %exitcond23, label %for.body.us, label %for.end8.loopexit
; CHECK: for.cond1.for.inc6_crit_edge.us:
; CHECK:   %inc7.us = add nuw i32 %i.022.us, 1
; CHECK:   %exitcond23 = icmp ne i32 %inc7.us, %flatten.tripcount
; CHECK:   br i1 %exitcond23, label %for.body.us, label %for.end8.loopexit

for.end8.loopexit:                                ; preds = %for.cond1.for.inc6_crit_edge.us
  br label %for.end8

for.end8:                                         ; preds = %for.end8.loopexit, %entry
  %i.0.lcssa = phi i32 [ 0, %entry ], [ %N, %for.end8.loopexit ]
  ret i32 %i.0.lcssa
}