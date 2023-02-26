; RUN: opt < %s -S -loop-flatten -verify-loop-info -verify-dom-info -verify-scev -verify | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-v128:64:128-a:0:32-n32-S64"

; CHECK-LABEL: test7
; Various inner phis and conditions which we can still work with
define signext i16 @test7(i32 %I, i32 %J, i32* nocapture readonly %C, i16 signext %limit) {
entry:
  %cmp43 = icmp eq i32 %J, 0
  br i1 %cmp43, label %for.end17, label %for.body.lr.ph

for.body.lr.ph:                                   ; preds = %entry
  %conv = sext i16 %limit to i32
  br label %for.body.us
; CHECK: for.body.lr.ph:
; CHECK:   %conv = sext i16 %limit to i32
; CHECK:   %flatten.tripcount = mul i32 %J, %J
; CHECK:   br label %for.body.us

for.body.us:                                      ; preds = %for.cond1.for.inc15_crit_edge.us, %for.body.lr.ph
  %i.047.us = phi i32 [ 0, %for.body.lr.ph ], [ %inc16.us, %for.cond1.for.inc15_crit_edge.us ]
  %ret.046.us = phi i16 [ 0, %for.body.lr.ph ], [ %ret.2.us.lcssa, %for.cond1.for.inc15_crit_edge.us ]
  %prev.045.us = phi i32 [ 0, %for.body.lr.ph ], [ %.lcssa, %for.cond1.for.inc15_crit_edge.us ]
  %tmp.044.us = phi i32 [ 0, %for.body.lr.ph ], [ %tmp.2.us.lcssa, %for.cond1.for.inc15_crit_edge.us ]
  %mul.us = mul i32 %i.047.us, %J
  br label %for.body3.us
; CHECK: for.body.us:
; CHECK:   %i.047.us = phi i32 [ 0, %for.body.lr.ph ], [ %inc16.us, %for.cond1.for.inc15_crit_edge.us ]
; CHECK:   %ret.046.us = phi i16 [ 0, %for.body.lr.ph ], [ %ret.2.us.lcssa, %for.cond1.for.inc15_crit_edge.us ]
; CHECK:   %prev.045.us = phi i32 [ 0, %for.body.lr.ph ], [ %.lcssa, %for.cond1.for.inc15_crit_edge.us ]
; CHECK:   %tmp.044.us = phi i32 [ 0, %for.body.lr.ph ], [ %tmp.2.us.lcssa, %for.cond1.for.inc15_crit_edge.us ]
; CHECK:   %mul.us = mul i32 %i.047.us, %J
; CHECK:   br label %for.body3.us

for.body3.us:                                     ; preds = %for.body.us, %if.end.us
  %j.040.us = phi i32 [ 0, %for.body.us ], [ %inc.us, %if.end.us ]
  %ret.139.us = phi i16 [ %ret.046.us, %for.body.us ], [ %ret.2.us, %if.end.us ]
  %prev.138.us = phi i32 [ %prev.045.us, %for.body.us ], [ %0, %if.end.us ]
  %tmp.137.us = phi i32 [ %tmp.044.us, %for.body.us ], [ %tmp.2.us, %if.end.us ]
  %add.us = add i32 %j.040.us, %mul.us
  %arrayidx.us = getelementptr inbounds i32, i32* %C, i32 %add.us
  %0 = load i32, i32* %arrayidx.us, align 4
  %add4.us = add nsw i32 %0, %tmp.137.us
  %cmp5.us = icmp sgt i32 %add4.us, %conv
  br i1 %cmp5.us, label %if.then.us, label %if.else.us
; CHECK: for.body3.us:
; CHECK:   %j.040.us = phi i32 [ 0, %for.body.us ]
; CHECK:   %ret.139.us = phi i16 [ %ret.046.us, %for.body.us ]
; CHECK:   %prev.138.us = phi i32 [ %prev.045.us, %for.body.us ]
; CHECK:   %tmp.137.us = phi i32 [ %tmp.044.us, %for.body.us ]
; CHECK:   %add.us = add i32 %j.040.us, %mul.us
; CHECK:   %arrayidx.us = getelementptr inbounds i32, i32* %C, i32 %i.047.us
; CHECK:   %0 = load i32, i32* %arrayidx.us, align 4
; CHECK:   %add4.us = add nsw i32 %0, %tmp.137.us
; CHECK:   %cmp5.us = icmp sgt i32 %add4.us, %conv
; CHECK:   br i1 %cmp5.us, label %if.then.us, label %if.else.us

if.else.us:                                       ; preds = %for.body3.us
  %cmp10.us = icmp sgt i32 %0, %prev.138.us
  %cond.us = zext i1 %cmp10.us to i32
  %conv1235.us = zext i16 %ret.139.us to i32
  %add13.us = add nuw nsw i32 %cond.us, %conv1235.us
  br label %if.end.us
; CHECK: if.else.us:
; CHECK:   %cmp10.us = icmp sgt i32 %0, %prev.138.us
; CHECK:   %cond.us = zext i1 %cmp10.us to i32
; CHECK:   %conv1235.us = zext i16 %ret.139.us to i32
; CHECK:   %add13.us = add nuw nsw i32 %cond.us, %conv1235.us
; CHECK:   br label %if.end.us

if.then.us:                                       ; preds = %for.body3.us
  %conv7.us = sext i16 %ret.139.us to i32
  %add8.us = add nsw i32 %conv7.us, 10
  br label %if.end.us
; CHECK: if.then.us:
; CHECK:   %conv7.us = sext i16 %ret.139.us to i32
; CHECK:   %add8.us = add nsw i32 %conv7.us, 10
; CHECK:   br label %if.end.us

if.end.us:                                        ; preds = %if.then.us, %if.else.us
  %tmp.2.us = phi i32 [ 0, %if.then.us ], [ %add4.us, %if.else.us ]
  %ret.2.in.us = phi i32 [ %add8.us, %if.then.us ], [ %add13.us, %if.else.us ]
  %ret.2.us = trunc i32 %ret.2.in.us to i16
  %inc.us = add nuw i32 %j.040.us, 1
  %exitcond = icmp ne i32 %inc.us, %J
  br i1 %exitcond, label %for.body3.us, label %for.cond1.for.inc15_crit_edge.us
; CHECK: if.end.us:
; CHECK:   %tmp.2.us = phi i32 [ 0, %if.then.us ], [ %add4.us, %if.else.us ]
; CHECK:   %ret.2.in.us = phi i32 [ %add8.us, %if.then.us ], [ %add13.us, %if.else.us ]
; CHECK:   %ret.2.us = trunc i32 %ret.2.in.us to i16
; CHECK:   %inc.us = add nuw i32 %j.040.us, 1
; CHECK:   %exitcond = icmp ne i32 %inc.us, %J
; CHECK:   br label %for.cond1.for.inc15_crit_edge.us

for.cond1.for.inc15_crit_edge.us:                 ; preds = %if.end.us
  %tmp.2.us.lcssa = phi i32 [ %tmp.2.us, %if.end.us ]
  %ret.2.us.lcssa = phi i16 [ %ret.2.us, %if.end.us ]
  %.lcssa = phi i32 [ %0, %if.end.us ]
  %inc16.us = add nuw i32 %i.047.us, 1
  %exitcond49 = icmp ne i32 %inc16.us, %J
  br i1 %exitcond49, label %for.body.us, label %for.end17.loopexit
; CHECK: for.cond1.for.inc15_crit_edge.us:
; CHECK:   %tmp.2.us.lcssa = phi i32 [ %tmp.2.us, %if.end.us ]
; CHECK:   %ret.2.us.lcssa = phi i16 [ %ret.2.us, %if.end.us ]
; CHECK:   %.lcssa = phi i32 [ %0, %if.end.us ]
; CHECK:   %inc16.us = add nuw i32 %i.047.us, 1
; CHECK:   %exitcond49 = icmp ne i32 %inc16.us, %flatten.tripcount
; CHECK:   br i1 %exitcond49, label %for.body.us, label %for.end17.loopexit

for.end17.loopexit:                               ; preds = %for.cond1.for.inc15_crit_edge.us
  %ret.2.us.lcssa.lcssa = phi i16 [ %ret.2.us.lcssa, %for.cond1.for.inc15_crit_edge.us ]
  br label %for.end17

for.end17:                                        ; preds = %for.end17.loopexit, %entry
  %ret.0.lcssa = phi i16 [ 0, %entry ], [ %ret.2.us.lcssa.lcssa, %for.end17.loopexit ]
  ret i16 %ret.0.lcssa
}