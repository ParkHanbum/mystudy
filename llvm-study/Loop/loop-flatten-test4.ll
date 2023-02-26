; RUN: opt < %s -S -loop-flatten -verify-loop-info -verify-dom-info -verify-scev -verify | FileCheck %s

target datalayout = "e-m:e-p:32:32-i64:64-v128:64:128-a:0:32-n32-S64"


; CHECK-LABEL: test4
; Multiplication cannot overflow, so we can replace the original loop.
define void @test4(i16 zeroext %N, i32* nocapture %C, i32* nocapture readonly %A, i32 %scale) {
entry:
  %conv = zext i16 %N to i32
  %cmp30 = icmp eq i16 %N, 0
  br i1 %cmp30, label %for.cond.cleanup, label %for.body.lr.ph.split.us
; CHECK: entry:
; CHECK: %[[LIMIT:.*]] = zext i16 %N to i32
; CHECK: br i1 %{{.*}} label %for.cond.cleanup, label %for.body.lr.ph.split.us

for.body.lr.ph.split.us:                          ; preds = %entry
  br label %for.body.us
; CHECK: for.body.lr.ph.split.us:
; CHECK: %[[TRIPCOUNT:.*]] = mul i32 %[[LIMIT]], %[[LIMIT]]
; CHECK: br label %for.body.us

for.body.us:                                      ; preds = %for.cond2.for.cond.cleanup6_crit_edge.us, %for.body.lr.ph.split.us
  %i.031.us = phi i32 [ 0, %for.body.lr.ph.split.us ], [ %inc15.us, %for.cond2.for.cond.cleanup6_crit_edge.us ]
  %mul.us = mul nuw nsw i32 %i.031.us, %conv
  br label %for.body7.us
; CHECK: for.body.us:
; CHECK: %[[OUTER_IV:.*]] = phi i32
; CHECK: br label %for.body7.us

for.body7.us:                                     ; preds = %for.body.us, %for.body7.us
; CHECK: for.body7.us:
  %j.029.us = phi i32 [ 0, %for.body.us ], [ %inc.us, %for.body7.us ]
  %add.us = add nuw nsw i32 %j.029.us, %mul.us
  %arrayidx.us = getelementptr inbounds i32, i32* %A, i32 %add.us
; CHECK: getelementptr inbounds i32, i32* %A, i32 %[[OUTER_IV]]
  %0 = load i32, i32* %arrayidx.us, align 4
  %mul9.us = mul nsw i32 %0, %scale
; CHECK: getelementptr inbounds i32, i32* %C, i32 %[[OUTER_IV]]
  %arrayidx13.us = getelementptr inbounds i32, i32* %C, i32 %add.us
  store i32 %mul9.us, i32* %arrayidx13.us, align 4
  %inc.us = add nuw nsw i32 %j.029.us, 1
  %exitcond = icmp ne i32 %inc.us, %conv
  br i1 %exitcond, label %for.body7.us, label %for.cond2.for.cond.cleanup6_crit_edge.us
; CHECK: br label %for.cond2.for.cond.cleanup6_crit_edge.us

for.cond2.for.cond.cleanup6_crit_edge.us:         ; preds = %for.body7.us
  %inc15.us = add nuw nsw i32 %i.031.us, 1
  %exitcond32 = icmp ne i32 %inc15.us, %conv
  br i1 %exitcond32, label %for.body.us, label %for.cond.cleanup.loopexit
; CHECK: for.cond2.for.cond.cleanup6_crit_edge.us:
; CHECK: br i1 %exitcond32, label %for.body.us, label %for.cond.cleanup.loopexit

for.cond.cleanup.loopexit:                        ; preds = %for.cond2.for.cond.cleanup6_crit_edge.us
  br label %for.cond.cleanup
; CHECK: for.cond.cleanup.loopexit:
; CHECK: br label %for.cond.cleanup

for.cond.cleanup:                                 ; preds = %for.cond.cleanup.loopexit, %entry
  ret void
; CHECK: for.cond.cleanup:
; CHECK: ret void
}

