define i32 @bitsize_mismatch_1(i32 %x.coerce) {
entry:
  %inc = add i32 %x.coerce, 1
  %bf.value = and i32 %inc, 127
  %0 = and i32 %x.coerce, -128
  %bf.shl = add i32 %0, 128
  %bf.set8 = or disjoint i32 %bf.value, %bf.shl
  ret i32 %bf.set8
}

define i8 @src_2_bitfield_const(i8 %x.coerce) {
entry:
  %inc = add i8 %x.coerce, 1
  %bf.value = and i8 %inc, 63
  %0 = and i8 %x.coerce, -64
  %bf.shl = add i8 %0, 64
  %bf.set8 = or disjoint i8 %bf.value, %bf.shl
  ret i8 %bf.set8
}

define i8 @src_3_bitfield_op(i8 %x.coerce, i8 %y.coerce) {
entry:
  %narrow = add i8 %y.coerce, %x.coerce
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x.coerce, 24
  %bf.lshr1244 = add i8 %bf.lshr, %y.coerce
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x.coerce, -32
  %bf.lshr2547 = add i8 %bf.lshr22, %y.coerce
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}
;  %0 = and i8 %x.coerce, 11
;  %1 = add nuw i8 %0, 8
;  %2 = and i8 %x.coerce, 20
;  %3 = xor i8 %1, %2
define i8 @src_3_bitfield_const(i8 %x.coerce) {
entry:
  %narrow = add i8 %x.coerce, 1
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x.coerce, 24
  %bf.lshr1244 = add i8 %bf.lshr, 8
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x.coerce, -32
  %bf.lshr2547 = add i8 %bf.lshr22, 32
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}
define i8 @src_3_bitfield_const_ic(i8 %x.coerce) {
entry:
  %narrow = add i8 %x.coerce, 1
  %bf.value = and i8 %narrow, 7
  %bf.lshr1244 = add i8 %x.coerce, 8
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %0 = and i8 %x.coerce, -32
  %bf.value30 = add i8 %0, 32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @src_3_bitfield_const_ic_1opt(i8 %x.coerce) {
entry:
  %0 = and i8 %x.coerce, 11
  %1 = add nuw nsw i8 %0, 9
  %2 = and i8 %x.coerce, 20
  %bf_set20 = xor i8 %1, %2
  %3 = and i8 %x.coerce, -32
  %bf_set30 = add i8 %3, 32
  %bf_set33 = or disjoint i8 %bf_set20, %bf_set30
  ret i8 %bf_set33
}

define i8 @src_3_bitfield_const_mix(i8 %x.coerce, i8 %y.coerce) {
entry:
  %narrow = add i8 %y.coerce, %x.coerce
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x.coerce, 24
  %bf.lshr1244 = add i8 %bf.lshr, %y.coerce
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x.coerce, -32
  %bf.lshr2547 = add i8 %bf.lshr22, 32
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_1bit_low(i8 %x.coerce, i8 %y.coerce) {
entry:
  %narrow = add i8 %y.coerce, %x.coerce
  %bf.value = and i8 %narrow, 1
  %bf.lshr = and i8 %x.coerce, 24
  %bf.lshr1244 = add i8 %bf.lshr, %y.coerce
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x.coerce, -32
  %bf.lshr2547 = add i8 %bf.lshr22, %y.coerce
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_1bit_mid(i8 %x.coerce, i8 %y.coerce) {
entry:
  %narrow = add i8 %y.coerce, %x.coerce
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x.coerce, 8
  %bf.lshr1244 = add i8 %bf.lshr, %y.coerce
  %bf.shl = and i8 %bf.lshr1244, 8
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x.coerce, -16
  %bf.lshr2547 = add i8 %bf.lshr22, %y.coerce
  %bf.value30 = and i8 %bf.lshr2547, -16
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_1bit_high(i8 %x.coerce, i8 %y.coerce) {
entry:
  %narrow = add i8 %y.coerce, %x.coerce
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x.coerce, 24
  %bf.lshr1244 = add i8 %bf.lshr, %y.coerce
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x.coerce, 32
  %bf.lshr2547 = add i8 %bf.lshr22, %y.coerce
  %bf.value30 = and i8 %bf.lshr2547, 32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_over_mid_upper(i8 %x.coerce, i8 %y.coerce) {
entry:
  %narrow = add i8 %y.coerce, %x.coerce
  %bf.value = and i8 %narrow, 17
  %bf.lshr = and i8 %x.coerce, 24
  %bf.lshr1244 = add i8 %bf.lshr, %y.coerce
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x.coerce, -32
  %bf.lshr2547 = add i8 %bf.lshr22, %y.coerce
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_over_high_upper(i8 %x.coerce, i8 %y.coerce) {
entry:
  %narrow = add i8 %y.coerce, %x.coerce
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x.coerce, 56
  %bf.lshr1244 = add i8 %bf.lshr, %y.coerce
  %bf.shl = and i8 %bf.lshr1244, 56
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x.coerce, -32
  %bf.lshr2547 = add i8 %bf.lshr22, %y.coerce
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_under_mid_lower(i8 %x.coerce, i8 %y.coerce) {
entry:
  %narrow = add i8 %y.coerce, %x.coerce
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x.coerce, 28
  %bf.lshr1244 = add i8 %bf.lshr, %y.coerce
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x.coerce, -32
  %bf.lshr2547 = add i8 %bf.lshr22, %y.coerce
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_under_high_lower(i8 %x.coerce, i8 %y.coerce) {
entry:
  %narrow = add i8 %y.coerce, %x.coerce
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x.coerce, 24
  %bf.lshr1244 = add i8 %bf.lshr, %y.coerce
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x.coerce, -16
  %bf.lshr2547 = add i8 %bf.lshr22, %y.coerce
  %bf.value30 = and i8 %bf.lshr2547, -16
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i32 @src_2_bitfield_op(i32 %x.coerce, i32 %y.coerce) {
; CHECK-LABEL: @src_2_bitfield_op(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[INC:%.*]] = add i32 [[X_COERCE:%.*]], [[Y_COERCE:%.*]]
; CHECK-NEXT:    [[BF_VALUE:%.*]] = and i32 [[INC]], 127
; CHECK-NEXT:    [[TMP0:%.*]] = and i32 [[X_COERCE]], [[Y_COERCE]]
; CHECK-NEXT:    [[BF_SHL:%.*]] = add i32 [[TMP0]], 128
; CHECK-NEXT:    [[BF_SET8:%.*]] = or disjoint i32 [[BF_VALUE]], [[BF_SHL]]
; CHECK-NEXT:    ret i32 [[BF_SET8]]
;
entry:
  %inc = add i32 %x.coerce, %y.coerce
  %bf.value = and i32 %inc, 127
  %0 = and i32 %x.coerce, -128
  %bf.shl = add i32 %0, %y.coerce
  %bf.set8 = or disjoint i32 %bf.value, %bf.shl
  ret i32 %bf.set8
}

