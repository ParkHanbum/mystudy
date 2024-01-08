; test or disjoint which used for BitField Arithmetic.
; Positive

define i8 @src_2_bitfield_op(i8 %x, i8 %y) {
; CHECK-LABEL: @src_2_bitfield_op(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 11
; CHECK-NEXT:    [[TMP1:%.*]] = and i8 [[Y:%.*]], 11
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i8 [[TMP0]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = xor i8 [[X]], [[Y]]
; CHECK-NEXT:    [[TMP4:%.*]] = and i8 [[TMP3]], 20
; CHECK-NEXT:    [[BF_SET20:%.*]] = xor i8 [[TMP2]], [[TMP4]]
; CHECK-NEXT:    ret i8 [[BF_SET20]]
;
entry:
  %narrow = add i8 %y, %x
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x, 24
  %bf.lshr1228 = add i8 %bf.lshr, %y
  %bf.shl = and i8 %bf.lshr1228, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  ret i8 %bf.set20
}

define i8 @src_2_bitfield_const(i8 %x) {
; CHECK-LABEL: @src_2_bitfield_const(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 95
; CHECK-NEXT:    [[TMP1:%.*]] = add nuw i8 [[TMP0]], 65
; CHECK-NEXT:    [[TMP2:%.*]] = and i8 [[X]], -96
; CHECK-NEXT:    [[BF_SET8:%.*]] = xor i8 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    ret i8 [[BF_SET8]]
;
entry:
  %inc = add i8 %x, 1
  %bf.value = and i8 %inc, 63
  %0 = and i8 %x, -64
  %bf.shl = add i8 %0, 64
  %bf.set8 = or disjoint i8 %bf.value, %bf.shl
  ret i8 %bf.set8
}

define i8 @src_3_bitfield_op(i8 %x, i8 %y) {
; CHECK-LABEL: @src_3_bitfield_op(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 107
; CHECK-NEXT:    [[TMP1:%.*]] = and i8 [[Y:%.*]], 107
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw i8 [[TMP0]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = xor i8 [[X]], [[Y]]
; CHECK-NEXT:    [[TMP4:%.*]] = and i8 [[TMP3]], -108
; CHECK-NEXT:    [[BF_SET33:%.*]] = xor i8 [[TMP2]], [[TMP4]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
;
entry:
  %narrow = add i8 %y, %x
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x, 24
  %bf.lshr1244 = add i8 %bf.lshr, %y
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x, -32
  %bf.lshr2547 = add i8 %bf.lshr22, %y
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @src_3_bitfield_const(i8 %x) {
; CHECK-LABEL: @src_3_bitfield_const(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 107
; CHECK-NEXT:    [[TMP1:%.*]] = add nuw i8 [[TMP0]], 41
; CHECK-NEXT:    [[TMP2:%.*]] = and i8 [[X]], -108
; CHECK-NEXT:    [[BF_SET33:%.*]] = xor i8 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
;
entry:
  %narrow = add i8 %x, 1
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x, 24
  %bf.lshr1244 = add i8 %bf.lshr, 8
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x, -32
  %bf.lshr2547 = add i8 %bf.lshr22, 32
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

; test or disjoint which used for BitField Arithmetic.
; Negative
define i8 @bitsize_mismatch_1(i8 %x) {
; CHECK-LABEL: @bitsize_mismatch_1(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 1
; CHECK-NEXT:    [[BF_VALUE:%.*]] = xor i8 [[TMP0]], 1
; CHECK-NEXT:    [[TMP1:%.*]] = and i8 [[X]], -127
; CHECK-NEXT:    [[BF_SHL:%.*]] = xor i8 [[TMP1]], -128
; CHECK-NEXT:    [[BF_SET8:%.*]] = or disjoint i8 [[BF_VALUE]], [[BF_SHL]]
; CHECK-NEXT:    ret i8 [[BF_SET8]]
;
entry:
  %inc = add i8 %x, 1
  %bf.value = and i8 %inc, 1
  %0 = and i8 %x, -127
  %bf.shl = add i8 %0, 128
  %bf.set8 = or disjoint i8 %bf.value, %bf.shl
  ret i8 %bf.set8
}

define i8 @bit_arithmetic_1bit_low(i8 %x, i8 %y) {
; CHECK-LABEL: @bit_arithmetic_1bit_low(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[NARROW:%.*]] = add i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[BF_VALUE:%.*]] = and i8 [[NARROW]], 1
; CHECK-NEXT:    [[BF_LSHR:%.*]] = and i8 [[X]], 24
; CHECK-NEXT:    [[BF_LSHR1244:%.*]] = add i8 [[BF_LSHR]], [[Y]]
; CHECK-NEXT:    [[BF_SHL:%.*]] = and i8 [[BF_LSHR1244]], 24
; CHECK-NEXT:    [[BF_SET20:%.*]] = or disjoint i8 [[BF_VALUE]], [[BF_SHL]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -32
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
;
entry:
  %narrow = add i8 %y, %x
  %bf.value = and i8 %narrow, 1
  %bf.lshr = and i8 %x, 24
  %bf.lshr1244 = add i8 %bf.lshr, %y
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x, -32
  %bf.lshr2547 = add i8 %bf.lshr22, %y
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_1bit_mid(i8 %x, i8 %y) {
; CHECK-LABEL: @bit_arithmetic_1bit_mid(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[NARROW:%.*]] = add i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[BF_VALUE:%.*]] = and i8 [[NARROW]], 7
; CHECK-NEXT:    [[BF_LSHR:%.*]] = and i8 [[X]], 8
; CHECK-NEXT:    [[BF_LSHR1244:%.*]] = add i8 [[BF_LSHR]], [[Y]]
; CHECK-NEXT:    [[BF_SHL:%.*]] = and i8 [[BF_LSHR1244]], 8
; CHECK-NEXT:    [[BF_SET20:%.*]] = or disjoint i8 [[BF_VALUE]], [[BF_SHL]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -16
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -16
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
;
entry:
  %narrow = add i8 %y, %x
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x, 8
  %bf.lshr1244 = add i8 %bf.lshr, %y
  %bf.shl = and i8 %bf.lshr1244, 8
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x, -16
  %bf.lshr2547 = add i8 %bf.lshr22, %y
  %bf.value30 = and i8 %bf.lshr2547, -16
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_1bit_high(i8 %x, i8 %y) {
; CHECK-LABEL: @bit_arithmetic_1bit_high(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 11
; CHECK-NEXT:    [[TMP1:%.*]] = and i8 [[Y:%.*]], 11
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i8 [[TMP0]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = xor i8 [[X]], [[Y]]
; CHECK-NEXT:    [[TMP4:%.*]] = and i8 [[TMP3]], 20
; CHECK-NEXT:    [[BF_SET20:%.*]] = xor i8 [[TMP2]], [[TMP4]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], 32
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], 32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
;
entry:
  %narrow = add i8 %y, %x
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x, 24
  %bf.lshr1244 = add i8 %bf.lshr, %y
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x, 32
  %bf.lshr2547 = add i8 %bf.lshr22, %y
  %bf.value30 = and i8 %bf.lshr2547, 32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_low_over_mid(i8 %x, i8 %y) {
; CHECK-LABEL: @bit_arithmetic_low_over_mid(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[NARROW:%.*]] = add i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[BF_VALUE:%.*]] = and i8 [[NARROW]], 17
; CHECK-NEXT:    [[BF_LSHR:%.*]] = and i8 [[X]], 24
; CHECK-NEXT:    [[BF_LSHR1244:%.*]] = add i8 [[BF_LSHR]], [[Y]]
; CHECK-NEXT:    [[BF_SHL:%.*]] = and i8 [[BF_LSHR1244]], 24
; CHECK-NEXT:    [[BF_SET20:%.*]] = or disjoint i8 [[BF_VALUE]], [[BF_SHL]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -32
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
;
entry:
  %narrow = add i8 %y, %x
  %bf.value = and i8 %narrow, 17
  %bf.lshr = and i8 %x, 24
  %bf.lshr1244 = add i8 %bf.lshr, %y
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x, -32
  %bf.lshr2547 = add i8 %bf.lshr22, %y
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_mid_over_high(i8 %x, i8 %y) {
; CHECK-LABEL: @bit_arithmetic_mid_over_high(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 27
; CHECK-NEXT:    [[TMP1:%.*]] = and i8 [[Y:%.*]], 27
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i8 [[TMP0]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = xor i8 [[X]], [[Y]]
; CHECK-NEXT:    [[TMP4:%.*]] = and i8 [[TMP3]], 36
; CHECK-NEXT:    [[BF_SET20:%.*]] = xor i8 [[TMP2]], [[TMP4]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -32
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
;
entry:
  %narrow = add i8 %y, %x
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x, 56
  %bf.lshr1244 = add i8 %bf.lshr, %y
  %bf.shl = and i8 %bf.lshr1244, 56
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x, -32
  %bf.lshr2547 = add i8 %bf.lshr22, %y
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_mid_under_lower(i8 %x, i8 %y) {
; CHECK-LABEL: @bit_arithmetic_mid_under_lower(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[NARROW:%.*]] = add i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[BF_VALUE:%.*]] = and i8 [[NARROW]], 7
; CHECK-NEXT:    [[BF_LSHR:%.*]] = and i8 [[X]], 28
; CHECK-NEXT:    [[BF_LSHR1244:%.*]] = add i8 [[BF_LSHR]], [[Y]]
; CHECK-NEXT:    [[BF_SHL:%.*]] = and i8 [[BF_LSHR1244]], 24
; CHECK-NEXT:    [[BF_SET20:%.*]] = or disjoint i8 [[BF_VALUE]], [[BF_SHL]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -32
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
;
entry:
  %narrow = add i8 %y, %x
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x, 28
  %bf.lshr1244 = add i8 %bf.lshr, %y
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x, -32
  %bf.lshr2547 = add i8 %bf.lshr22, %y
  %bf.value30 = and i8 %bf.lshr2547, -32
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}

define i8 @bit_arithmetic_high_under_mid(i8 %x, i8 %y) {
; CHECK-LABEL: @bit_arithmetic_high_under_mid(
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 11
; CHECK-NEXT:    [[TMP1:%.*]] = and i8 [[Y:%.*]], 11
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i8 [[TMP0]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = xor i8 [[X]], [[Y]]
; CHECK-NEXT:    [[TMP4:%.*]] = and i8 [[TMP3]], 20
; CHECK-NEXT:    [[BF_SET20:%.*]] = xor i8 [[TMP2]], [[TMP4]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -16
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -16
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
;
entry:
  %narrow = add i8 %y, %x
  %bf.value = and i8 %narrow, 7
  %bf.lshr = and i8 %x, 24
  %bf.lshr1244 = add i8 %bf.lshr, %y
  %bf.shl = and i8 %bf.lshr1244, 24
  %bf.set20 = or disjoint i8 %bf.value, %bf.shl
  %bf.lshr22 = and i8 %x, -16
  %bf.lshr2547 = add i8 %bf.lshr22, %y
  %bf.value30 = and i8 %bf.lshr2547, -16
  %bf.set33 = or disjoint i8 %bf.set20, %bf.value30
  ret i8 %bf.set33
}







; test or disjoint which used for BitField Arithmetic.
; Positive
define i8 @tgt_2_bitfield_op(i8 %x, i8 %y) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 11
; CHECK-NEXT:    [[TMP1:%.*]] = and i8 [[Y:%.*]], 11
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i8 [[TMP0]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = xor i8 [[X]], [[Y]]
; CHECK-NEXT:    [[TMP4:%.*]] = and i8 [[TMP3]], 20
; CHECK-NEXT:    [[BF_SET20:%.*]] = xor i8 [[TMP2]], [[TMP4]]
; CHECK-NEXT:    ret i8 [[BF_SET20]]
}

define i8 @tgt_2_bitfield_const(i8 %x) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 11
; CHECK-NEXT:    [[TMP1:%.*]] = add nuw nsw i8 [[TMP0]], 9
; CHECK-NEXT:    [[TMP2:%.*]] = and i8 [[X]], 20
; CHECK-NEXT:    [[BF_SET20:%.*]] = xor i8 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    ret i8 [[BF_SET20]]
}

define i8 @tgt_3_bitfield_op(i8 %x, i8 %y) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 107
; CHECK-NEXT:    [[TMP1:%.*]] = and i8 [[Y:%.*]], 107
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw i8 [[TMP0]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = xor i8 [[X]], [[Y]]
; CHECK-NEXT:    [[TMP4:%.*]] = and i8 [[TMP3]], -108
; CHECK-NEXT:    [[BF_SET33:%.*]] = xor i8 [[TMP2]], [[TMP4]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}

define i8 @tgt_3_bitfield_const(i8 %x) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 107
; CHECK-NEXT:    [[TMP1:%.*]] = add nuw i8 [[TMP0]], 41
; CHECK-NEXT:    [[TMP2:%.*]] = and i8 [[X]], -108
; CHECK-NEXT:    [[BF_SET33:%.*]] = xor i8 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}

; test or disjoint which used for BitField Arithmetic.
; Negative
define i8 @src_bit_arithmetic_bitsize_1_low(i8 %x, i8 %y) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[NARROW:%.*]] = add i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[BF_VALUE:%.*]] = and i8 [[NARROW]], 1
; CHECK-NEXT:    [[BF_LSHR:%.*]] = and i8 [[X]], 30
; CHECK-NEXT:    [[BF_LSHR1244:%.*]] = add i8 [[BF_LSHR]], [[Y]]
; CHECK-NEXT:    [[BF_SHL:%.*]] = and i8 [[BF_LSHR1244]], 30
; CHECK-NEXT:    [[BF_SET20:%.*]] = or disjoint i8 [[BF_VALUE]], [[BF_SHL]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -32
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}

define i8 @tgt_bit_arithmetic_bitsize_1_mid(i8 %x, i8 %y) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[NARROW:%.*]] = add i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[BF_VALUE:%.*]] = and i8 [[NARROW]], 15
; CHECK-NEXT:    [[BF_LSHR:%.*]] = and i8 [[X]], 16
; CHECK-NEXT:    [[BF_LSHR1244:%.*]] = add i8 [[BF_LSHR]], [[Y]]
; CHECK-NEXT:    [[BF_SHL:%.*]] = and i8 [[BF_LSHR1244]], 16
; CHECK-NEXT:    [[BF_SET20:%.*]] = or disjoint i8 [[BF_VALUE]], [[BF_SHL]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -32
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}

define i8 @tgt_bit_arithmetic_bitsize_1_high(i8 %x, i8 %y) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 59
; CHECK-NEXT:    [[TMP1:%.*]] = and i8 [[Y:%.*]], 59
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i8 [[TMP0]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = xor i8 [[X]], [[Y]]
; CHECK-NEXT:    [[TMP4:%.*]] = and i8 [[TMP3]], 68
; CHECK-NEXT:    [[BF_SET20:%.*]] = xor i8 [[TMP2]], [[TMP4]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -128
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -128
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}

define i8 @tgt_bit_arithmetic_bitmask_low_over_mid(i8 %x, i8 %y) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[NARROW:%.*]] = add i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[BF_VALUE:%.*]] = and i8 [[NARROW]], 17
; CHECK-NEXT:    [[BF_LSHR:%.*]] = and i8 [[X]], 24
; CHECK-NEXT:    [[BF_LSHR1244:%.*]] = add i8 [[BF_LSHR]], [[Y]]
; CHECK-NEXT:    [[BF_SHL:%.*]] = and i8 [[BF_LSHR1244]], 24
; CHECK-NEXT:    [[BF_SET20:%.*]] = or disjoint i8 [[BF_VALUE]], [[BF_SHL]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -32
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}

define i8 @tgt_bit_arithmetic_bitmask_mid_over_high(i8 %x, i8 %y) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 27
; CHECK-NEXT:    [[TMP1:%.*]] = and i8 [[Y:%.*]], 27
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i8 [[TMP0]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = xor i8 [[X]], [[Y]]
; CHECK-NEXT:    [[TMP4:%.*]] = and i8 [[TMP3]], 36
; CHECK-NEXT:    [[BF_SET20:%.*]] = xor i8 [[TMP2]], [[TMP4]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -32
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}

define i8 @tgt_bit_arithmetic_bitmask_mid_under_lower(i8 %x, i8 %y) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[NARROW:%.*]] = add i8 [[Y:%.*]], [[X:%.*]]
; CHECK-NEXT:    [[BF_VALUE:%.*]] = and i8 [[NARROW]], 7
; CHECK-NEXT:    [[BF_LSHR:%.*]] = and i8 [[X]], 28
; CHECK-NEXT:    [[BF_LSHR1244:%.*]] = add i8 [[BF_LSHR]], [[Y]]
; CHECK-NEXT:    [[BF_SHL:%.*]] = and i8 [[BF_LSHR1244]], 24
; CHECK-NEXT:    [[BF_SET20:%.*]] = or disjoint i8 [[BF_VALUE]], [[BF_SHL]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -32
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}

define i8 @tgt_bit_arithmetic_bitmask_high_under_mid(i8 %x, i8 %y) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 11
; CHECK-NEXT:    [[TMP1:%.*]] = and i8 [[Y:%.*]], 11
; CHECK-NEXT:    [[TMP2:%.*]] = add nuw nsw i8 [[TMP0]], [[TMP1]]
; CHECK-NEXT:    [[TMP3:%.*]] = xor i8 [[X]], [[Y]]
; CHECK-NEXT:    [[TMP4:%.*]] = and i8 [[TMP3]], 20
; CHECK-NEXT:    [[BF_SET20:%.*]] = xor i8 [[TMP2]], [[TMP4]]
; CHECK-NEXT:    [[BF_LSHR22:%.*]] = and i8 [[X]], -16
; CHECK-NEXT:    [[BF_LSHR2547:%.*]] = add i8 [[BF_LSHR22]], [[Y]]
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = and i8 [[BF_LSHR2547]], -16
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}

define i8 @tgt_bit_arithmetic_addition_over_bitmask_low(i8 %x) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[BF_VALUE:%.*]] = and i8 [[X:%.*]], 7
; CHECK-NEXT:    [[BF_LSHR1244:%.*]] = add i8 [[X]], 8
; CHECK-NEXT:    [[BF_SHL:%.*]] = and i8 [[BF_LSHR1244]], 24
; CHECK-NEXT:    [[BF_SET20:%.*]] = or disjoint i8 [[BF_VALUE]], [[BF_SHL]]
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X]], -32
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = add i8 [[TMP0]], 32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}

define i8 @tgt_bit_arithmetic_addition_over_bitmask_mid(i8 %x) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[NARROW:%.*]] = add i8 [[X:%.*]], 1
; CHECK-NEXT:    [[BF_VALUE:%.*]] = and i8 [[NARROW]], 7
; CHECK-NEXT:    [[BF_LSHR:%.*]] = and i8 [[X]], 24
; CHECK-NEXT:    [[BF_SET20:%.*]] = or disjoint i8 [[BF_VALUE]], [[BF_LSHR]]
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X]], -32
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = add i8 [[TMP0]], 32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}

define i8 @tgt_bit_arithmetic_addition_under_bitmask_mid(i8 %x) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[NARROW:%.*]] = add i8 [[X:%.*]], 1
; CHECK-NEXT:    [[BF_VALUE:%.*]] = and i8 [[NARROW]], 7
; CHECK-NEXT:    [[BF_LSHR:%.*]] = and i8 [[X]], 24
; CHECK-NEXT:    [[BF_SET20:%.*]] = or disjoint i8 [[BF_VALUE]], [[BF_LSHR]]
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X]], -32
; CHECK-NEXT:    [[BF_VALUE30:%.*]] = add i8 [[TMP0]], 32
; CHECK-NEXT:    [[BF_SET33:%.*]] = or disjoint i8 [[BF_SET20]], [[BF_VALUE30]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}

define i8 @tgt_bit_arithmetic_addition_under_bitmask_high(i8 %x) {
; CHECK-NEXT:  entry:
; CHECK-NEXT:    [[TMP0:%.*]] = and i8 [[X:%.*]], 107
; CHECK-NEXT:    [[TMP1:%.*]] = add nuw i8 [[TMP0]], 41
; CHECK-NEXT:    [[TMP2:%.*]] = and i8 [[X]], -108
; CHECK-NEXT:    [[BF_SET33:%.*]] = xor i8 [[TMP1]], [[TMP2]]
; CHECK-NEXT:    ret i8 [[BF_SET33]]
}






; test or disjoint which used for BitField Arithmetic.
; Positive
define i8 @tgt_2_bitfield_op(i8 %x, i8 %y) {
entry:
  %0 = and i8 %x, 11
  %1 = and i8 %y, 11
  %2 = add nuw nsw i8 %0, %1
  %3 = xor i8 %x, %y
  %4 = and i8 %3, 20
  %bf_set20 = xor i8 %2, %4
  ret i8 %bf_set20
}

define i8 @tgt_2_bitfield_const(i8 %x) {
entry:
  %0 = and i8 %x, 11
  %1 = add nuw nsw i8 %0, 9
  %2 = and i8 %x, 20
  %bf_set20 = xor i8 %1, %2
  ret i8 %bf_set20
}

define i8 @tgt_3_bitfield_op(i8 %x, i8 %y) {
entry:
  %0 = and i8 %x, 107
  %1 = and i8 %y, 107
  %2 = add nuw i8 %0, %1
  %3 = xor i8 %x, %y
  %4 = and i8 %3, -108
  %bf_set33 = xor i8 %2, %4
  ret i8 %bf_set33
}

define i8 @tgt_3_bitfield_const(i8 %x) {
entry:
  %0 = and i8 %x, 107
  %1 = add nuw i8 %0, 41
  %2 = and i8 %x, -108
  %bf_set33 = xor i8 %1, %2
  ret i8 %bf_set33
}

; test or disjoint which used for BitField Arithmetic.
; Negative
define i8 @tgt_bit_arithmetic_bitsize_1_low(i8 %x, i8 %y) {
entry:
  %narrow = add i8 %y, %x
  %bf_value = and i8 %narrow, 1
  %bf_lshr = and i8 %x, 30
  %bf_lshr1244 = add i8 %bf_lshr, %y
  %bf_shl = and i8 %bf_lshr1244, 30
  %bf_set20 = or disjoint i8 %bf_value, %bf_shl
  %bf_lshr22 = and i8 %x, -32
  %bf_lshr2547 = add i8 %bf_lshr22, %y
  %bf_value30 = and i8 %bf_lshr2547, -32
  %bf_set33 = or disjoint i8 %bf_set20, %bf_value30
  ret i8 %bf_set33
}

define i8 @tgt_bit_arithmetic_bitsize_1_mid(i8 %x, i8 %y) {
entry:
  %narrow = add i8 %y, %x
  %bf_value = and i8 %narrow, 15
  %bf_lshr = and i8 %x, 16
  %bf_lshr1244 = add i8 %bf_lshr, %y
  %bf_shl = and i8 %bf_lshr1244, 16
  %bf_set20 = or disjoint i8 %bf_value, %bf_shl
  %bf_lshr22 = and i8 %x, -32
  %bf_lshr2547 = add i8 %bf_lshr22, %y
  %bf_value30 = and i8 %bf_lshr2547, -32
  %bf_set33 = or disjoint i8 %bf_set20, %bf_value30
  ret i8 %bf_set33
}

define i8 @tgt_bit_arithmetic_bitsize_1_high(i8 %x, i8 %y) {
entry:
  %0 = and i8 %x, 59
  %1 = and i8 %y, 59
  %2 = add nuw nsw i8 %0, %1
  %3 = xor i8 %x, %y
  %4 = and i8 %3, 68
  %bf_set20 = xor i8 %2, %4
  %bf_lshr22 = and i8 %x, -128
  %bf_lshr2547 = add i8 %bf_lshr22, %y
  %bf_value30 = and i8 %bf_lshr2547, -128
  %bf_set33 = or disjoint i8 %bf_set20, %bf_value30
  ret i8 %bf_set33
}

define i8 @tgt_bit_arithmetic_bitmask_low_over_mid(i8 %x, i8 %y) {
entry:
  %narrow = add i8 %y, %x
  %bf_value = and i8 %narrow, 17
  %bf_lshr = and i8 %x, 24
  %bf_lshr1244 = add i8 %bf_lshr, %y
  %bf_shl = and i8 %bf_lshr1244, 24
  %bf_set20 = or disjoint i8 %bf_value, %bf_shl
  %bf_lshr22 = and i8 %x, -32
  %bf_lshr2547 = add i8 %bf_lshr22, %y
  %bf_value30 = and i8 %bf_lshr2547, -32
  %bf_set33 = or disjoint i8 %bf_set20, %bf_value30
  ret i8 %bf_set33
}

define i8 @tgt_bit_arithmetic_bitmask_mid_over_high(i8 %x, i8 %y) {
entry:
  %0 = and i8 %x, 27
  %1 = and i8 %y, 27
  %2 = add nuw nsw i8 %0, %1
  %3 = xor i8 %x, %y
  %4 = and i8 %3, 36
  %bf_set20 = xor i8 %2, %4
  %bf_lshr22 = and i8 %x, -32
  %bf_lshr2547 = add i8 %bf_lshr22, %y
  %bf_value30 = and i8 %bf_lshr2547, -32
  %bf_set33 = or disjoint i8 %bf_set20, %bf_value30
  ret i8 %bf_set33
}

define i8 @tgt_bit_arithmetic_bitmask_mid_under_lower(i8 %x, i8 %y) {
entry:
  %narrow = add i8 %y, %x
  %bf_value = and i8 %narrow, 7
  %bf_lshr = and i8 %x, 28
  %bf_lshr1244 = add i8 %bf_lshr, %y
  %bf_shl = and i8 %bf_lshr1244, 24
  %bf_set20 = or disjoint i8 %bf_value, %bf_shl
  %bf_lshr22 = and i8 %x, -32
  %bf_lshr2547 = add i8 %bf_lshr22, %y
  %bf_value30 = and i8 %bf_lshr2547, -32
  %bf_set33 = or disjoint i8 %bf_set20, %bf_value30
  ret i8 %bf_set33
}

define i8 @tgt_bit_arithmetic_bitmask_high_under_mid(i8 %x, i8 %y) {
entry:
  %0 = and i8 %x, 11
  %1 = and i8 %y, 11
  %2 = add nuw nsw i8 %0, %1
  %3 = xor i8 %x, %y
  %4 = and i8 %3, 20
  %bf_set20 = xor i8 %2, %4
  %bf_lshr22 = and i8 %x, -16
  %bf_lshr2547 = add i8 %bf_lshr22, %y
  %bf_value30 = and i8 %bf_lshr2547, -16
  %bf_set33 = or disjoint i8 %bf_set20, %bf_value30
  ret i8 %bf_set33
}

define i8 @tgt_bit_arithmetic_addition_over_bitmask_low(i8 %x) {
entry:
  %bf_value = and i8 %x, 7
  %bf_lshr1244 = add i8 %x, 8
  %bf_shl = and i8 %bf_lshr1244, 24
  %bf_set20 = or disjoint i8 %bf_value, %bf_shl
  %0 = and i8 %x, -32
  %bf_value30 = add i8 %0, 32
  %bf_set33 = or disjoint i8 %bf_set20, %bf_value30
  ret i8 %bf_set33
}

define i8 @tgt_bit_arithmetic_addition_over_bitmask_mid(i8 %x) {
entry:
  %narrow = add i8 %x, 1
  %bf_value = and i8 %narrow, 7
  %bf_lshr = and i8 %x, 24
  %bf_set20 = or disjoint i8 %bf_value, %bf_lshr
  %0 = and i8 %x, -32
  %bf_value30 = add i8 %0, 32
  %bf_set33 = or disjoint i8 %bf_set20, %bf_value30
  ret i8 %bf_set33
}

define i8 @tgt_bit_arithmetic_addition_under_bitmask_mid(i8 %x) {
entry:
  %narrow = add i8 %x, 1
  %bf_value = and i8 %narrow, 7
  %bf_lshr = and i8 %x, 24
  %bf_set20 = or disjoint i8 %bf_value, %bf_lshr
  %0 = and i8 %x, -32
  %bf_value30 = add i8 %0, 32
  %bf_set33 = or disjoint i8 %bf_set20, %bf_value30
  ret i8 %bf_set33
}

define i8 @tgt_bit_arithmetic_addition_under_bitmask_high(i8 %x) {
entry:
  %0 = and i8 %x, 11
  %1 = add nuw nsw i8 %0, 9
  %2 = and i8 %x, 20
  %bf_set20 = xor i8 %1, %2
  %bf_lshr22 = and i8 %x, -32
  %bf_set33 = or disjoint i8 %bf_set20, %bf_lshr22
  ret i8 %bf_set33
}
