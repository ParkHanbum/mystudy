
target datalayout = "e-m:e-p:32:32-i64:64-v128:64:128-a:0:32-n32-S64"

; CHECK-LABEL: test1
; Simple loop where the IV's is constant
define i32 @test1(i32 %val, i16* nocapture %A) {
entry:
  br label %for.body
; CHECK: entry:
; CHECK:   %flatten.tripcount = mul i32 20, 10
; CHECK:   br label %for.body

for.body:                                         ; preds = %entry, %for.inc6
  %i.018 = phi i32 [ 0, %entry ], [ %inc7, %for.inc6 ]
  %mul = mul nuw nsw i32 %i.018, 20
  br label %for.body3
; CHECK: for.body:
; CHECK:   %i.018 = phi i32 [ 0, %entry ], [ %inc7, %for.inc6 ]
; CHECK:   %mul = mul nuw nsw i32 %i.018, 20
; CHECK:   br label %for.body3

for.body3:                                        ; preds = %for.body, %for.body3
  %j.017 = phi i32 [ 0, %for.body ], [ %inc, %for.body3 ]
  %add = add nuw nsw i32 %j.017, %mul
  %arrayidx = getelementptr inbounds i16, i16* %A, i32 %add
  %0 = load i16, i16* %arrayidx, align 2
  %conv16 = zext i16 %0 to i32
  %add4 = add i32 %conv16, %val
  %conv5 = trunc i32 %add4 to i16
  store i16 %conv5, i16* %arrayidx, align 2
  %inc = add nuw nsw i32 %j.017, 1
  %exitcond = icmp ne i32 %inc, 20
  br i1 %exitcond, label %for.body3, label %for.inc6
; CHECK: for.body3:
; CHECK:   %j.017 = phi i32 [ 0, %for.body ]
; CHECK:   %add = add nuw nsw i32 %j.017, %mul
; CHECK:   %arrayidx = getelementptr inbounds i16, i16* %A, i32 %i.018
; CHECK:   %0 = load i16, i16* %arrayidx, align 2
; CHECK:   %conv16 = zext i16 %0 to i32
; CHECK:   %add4 = add i32 %conv16, %val
; CHECK:   %conv5 = trunc i32 %add4 to i16
; CHECK:   store i16 %conv5, i16* %arrayidx, align 2
; CHECK:   %inc = add nuw nsw i32 %j.017, 1
; CHECK:   %exitcond = icmp ne i32 %inc, 20
; CHECK:   br label %for.inc6

for.inc6:                                         ; preds = %for.body3
  %inc7 = add nuw nsw i32 %i.018, 1
  %exitcond19 = icmp ne i32 %inc7, 10
  br i1 %exitcond19, label %for.body, label %for.end8
; CHECK: for.inc6:
; CHECK:   %inc7 = add nuw nsw i32 %i.018, 1
; CHECK:   %exitcond19 = icmp ne i32 %inc7, %flatten.tripcount
; CHECK:   br i1 %exitcond19, label %for.body, label %for.end8

for.end8:                                         ; preds = %for.inc6
  ret i32 10
}