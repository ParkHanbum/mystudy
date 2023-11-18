
declare void @use_i32(i32)

define i1 @icmp_eq_or_zext_zext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_eq_or_zext_zext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = zext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[OR:%.*]] = or i1 [[X]], [[Y]]
; CHECK-NEXT:    [[R:%.*]] = xor i1 [[OR:%.*]], true
; CHECK-NEXT:    ret i1 [[R]] 
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = zext i1 %y to i32
  call void @use_i32(i32 %zy)
  %or = or i32 %zx, %zy
  %r = icmp eq i32 %or, 0
  ret i1 %r
}

define i1 @icmp_eq_or_sext_sext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_eq_or_sext_sext(
; CHECK-NEXT:    [[ZX:%.*]] = sext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = sext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[OR:%.*]] = or i1 [[X]], [[Y]]
; CHECK-NEXT:    [[R:%.*]] = xor i1 [[OR:%.*]], true
; CHECK-NEXT:    ret i1 [[R]] 
  %zx = sext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = sext i1 %y to i32
  call void @use_i32(i32 %zy)
  %or = or i32 %zx, %zy
  %r = icmp eq i32 %or, 0
  ret i1 %r
}

define i1 @icmp_eq_or_zext_sext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_eq_or_zext_sext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = sext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[OR:%.*]] = or i1 [[X]], [[Y]]
; CHECK-NEXT:    [[R:%.*]] = xor i1 [[OR1:%.*]], true
; CHECK-NEXT:    ret i1 [[R]] 
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = sext i1 %y to i32
  call void @use_i32(i32 %zy)
  %or = or i32 %zx, %zy
  %r = icmp eq i32 %or, 0
  ret i1 %r
}

define i1 @icmp_eq_or_use_zext_zext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_eq_or_use_zext_zext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = zext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[OR:%.*]] = or i32 [[ZX]], [[ZY]]
; CHECK-NEXT:    call void @use_i32(i32 [[OR]])
; CHECK-NEXT:    [[R:%.*]] = icmp eq i32 [[OR:%.*]], 0
; CHECK-NEXT:    ret i1 [[R]] 
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = zext i1 %y to i32
  call void @use_i32(i32 %zy)
  %or = or i32 %zx, %zy
  call void @use_i32(i32 %or)
  %r = icmp eq i32 %or, 0
  ret i1 %r
}

define i1 @icmp_eq_and_zext_zext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_eq_and_zext_zext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = zext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[AND:%.*]] = and i1 [[X]], [[Y]]
; CHECK-NEXT:    [[R:%.*]] = xor i1 [[AND:%.*]], true
; CHECK-NEXT:    ret i1 [[R]]
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = zext i1 %y to i32
  call void @use_i32(i32 %zy)
  %and = and i32 %zx, %zy
  %r = icmp eq i32 %and, 0
  ret i1 %r
}

define i1 @icmp_eq_and_sext_sext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_eq_and_sext_sext(
; CHECK-NEXT:    [[ZX:%.*]] = sext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = sext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[AND:%.*]] = and i1 [[X]], [[Y]]
; CHECK-NEXT:    [[R:%.*]] = xor i1 [[AND:%.*]], true
; CHECK-NEXT:    ret i1 [[R]]
  %zx = sext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = sext i1 %y to i32
  call void @use_i32(i32 %zy)
  %and = and i32 %zx, %zy
  %r = icmp eq i32 %and, 0
  ret i1 %r
}

define i1 @icmp_eq_and_zext_sext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_eq_and_zext_sext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = sext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[AND:%.*]] = and i1 [[X]], [[Y]]
; CHECK-NEXT:    [[R:%.*]] = xor i1 [[AND:%.*]], true
; CHECK-NEXT:    ret i1 [[R]]
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = sext i1 %y to i32
  call void @use_i32(i32 %zy)
  %and = and i32 %zx, %zy
  %r = icmp eq i32 %and, 0
  ret i1 %r
}

define i1 @icmp_eq_and_use_zext_zext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_eq_and_use_zext_zext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = zext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[AND:%.*]] = and i32 [[ZX]], [[ZY]]
; CHECK-NEXT:    call void @use_i32(i32 [[AND]])
; CHECK-NEXT:    [[R:%.*]] = icmp eq i32 [[AND:%.*]], 0
; CHECK-NEXT:    ret i1 [[R]]
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = zext i1 %y to i32
  call void @use_i32(i32 %zy)
  %and = and i32 %zx, %zy
  call void @use_i32(i32 %and)
  %r = icmp eq i32 %and, 0
  ret i1 %r
}

define i1 @icmp_ne_or_zext_zext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_ne_or_zext_zext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = zext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[OR:%.*]] = or i1 [[X]], [[Y]]
; CHECK-NEXT:    ret i1 [[OR]]
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = zext i1 %y to i32
  call void @use_i32(i32 %zy)
  %or = or i32 %zx, %zy
  %r = icmp ne i32 %or, 0
  ret i1 %r
}

define i1 @icmp_ne_or_sext_sext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_ne_or_sext_sext(
; CHECK-NEXT:    [[ZX:%.*]] = sext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = sext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[OR:%.*]] = or i1 [[X]], [[Y]]
; CHECK-NEXT:    ret i1 [[OR]]
  %zx = sext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = sext i1 %y to i32
  call void @use_i32(i32 %zy)
  %or = or i32 %zx, %zy
  %r = icmp ne i32 %or, 0
  ret i1 %r
}

define i1 @icmp_ne_or_zext_sext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_ne_or_zext_sext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = sext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[OR:%.*]] = or i1 [[X]], [[Y]]
; CHECK-NEXT:    ret i1 [[OR]] 
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = sext i1 %y to i32
  call void @use_i32(i32 %zy)
  %or = or i32 %zx, %zy
  %r = icmp ne i32 %or, 0
  ret i1 %r
}

define i1 @icmp_ne_and_zext_zext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_ne_and_zext_zext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = zext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[AND:%.*]] = and i1 [[X]], [[Y]]
; CHECK-NEXT:    ret i1 [[AND]] 
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = zext i1 %y to i32
  call void @use_i32(i32 %zy)
  %and = and i32 %zx, %zy
  %r = icmp ne i32 %and, 0
  ret i1 %r
}

define i1 @icmp_ne_and_sext_sext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_ne_and_sext_sext(
; CHECK-NEXT:    [[ZX:%.*]] = sext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = sext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[AND:%.*]] = and i1 [[X]], [[Y]]
; CHECK-NEXT:    ret i1 [[AND]] 
  %zx = sext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = sext i1 %y to i32
  call void @use_i32(i32 %zy)
  %and = and i32 %zx, %zy
  %r = icmp ne i32 %and, 0
  ret i1 %r
}

define i1 @icmp_ne_and_zext_sext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_ne_and_zext_sext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = sext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[AND:%.*]] = and i1 [[X]], [[Y]]
; CHECK-NEXT:    ret i1 [[AND]] 
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = sext i1 %y to i32
  call void @use_i32(i32 %zy)
  %and = and i32 %zx, %zy
  %r = icmp ne i32 %and, 0
  ret i1 %r
}




define i1 @icmp_eq1_or_zext_zext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_eq1_or_zext_zext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = zext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[OR:%.*]] = or i1 [[X]], [[Y]]
; CHECK-NEXT:    [[R:%.*]] = xor i1 [[OR:%.*]], true
; CHECK-NEXT:    ret i1 [[R]] 
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = zext i1 %y to i32
  call void @use_i32(i32 %zy)
  %or = or i32 %zx, %zy
  %r = icmp eq i32 %or, 1
  ret i1 %r
}

define i1 @icmp_eq1_or_sext_sext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_eq1_or_sext_sext(
; CHECK-NEXT:    [[ZX:%.*]] = sext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = sext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[OR:%.*]] = or i1 [[X]], [[Y]]
; CHECK-NEXT:    [[R:%.*]] = xor i1 [[OR:%.*]], true
; CHECK-NEXT:    ret i1 [[R]] 
  %zx = sext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = sext i1 %y to i32
  call void @use_i32(i32 %zy)
  %or = or i32 %zx, %zy
  %r = icmp eq i32 %or, 1
  ret i1 %r
}

define i1 @icmp_eq1_or_zext_sext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_eq_or_zext_sext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = sext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[OR:%.*]] = or i1 [[X]], [[Y]]
; CHECK-NEXT:    [[R:%.*]] = xor i1 [[OR1:%.*]], true
; CHECK-NEXT:    ret i1 [[R]] 
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = sext i1 %y to i32
  call void @use_i32(i32 %zy)
  %or = or i32 %zx, %zy
  %r = icmp eq i32 %or, 1
  ret i1 %r
}

define i1 @icmp_eq1_or_use_zext_zext(i1 %x, i1 %y) {
; CHECK-LABEL: @icmp_eq_or_use_zext_zext(
; CHECK-NEXT:    [[ZX:%.*]] = zext i1 [[X:%.*]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZX]])
; CHECK-NEXT:    [[ZY:%.*]] = zext i1 [[Y]] to i32
; CHECK-NEXT:    call void @use_i32(i32 [[ZY]])
; CHECK-NEXT:    [[OR:%.*]] = or i32 [[ZX]], [[ZY]]
; CHECK-NEXT:    call void @use_i32(i32 [[OR]])
; CHECK-NEXT:    [[R:%.*]] = icmp eq i32 [[OR:%.*]], 0
; CHECK-NEXT:    ret i1 [[R]] 
  %zx = zext i1 %x to i32
  call void @use_i32(i32 %zx)
  %zy = zext i1 %y to i32
  call void @use_i32(i32 %zy)
  %or = or i32 %zx, %zy
  call void @use_i32(i32 %or)
  %r = icmp eq i32 %or, 1
  ret i1 %r
}