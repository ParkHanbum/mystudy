define i32 @src(i32 %x.coerce) {
entry:
  %inc = add i32 %x.coerce, 1
  %bf.value = and i32 %inc, 127
  %0 = and i32 %x.coerce, -128
  %bf.shl = add i32 %0, 128
  %bf.set8 = or disjoint i32 %bf.value, %bf.shl
  ret i32 %bf.set8
}

define i32 @tgt(i32 %x.coerce) {
entry:
  %and = and i32 %x.coerce, 2147483583
  %add = add nuw i32 %and, 129
  %and6 = and i32 %x.coerce, -2147483584
  %xor7 = xor i32 %add, %and6
  ret i32 %xor7
}

;  %and = and i8 %y.coerce, 11
;  %add = add nuw i8 %and, %x.coerce
;  %and2 = and i8 %y.coerce, 20
;  %0 = xor i8 %add, %and2
define i8 @AddCounters(i8 %x.coerce, i8 %y.coerce) {
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


; and( add(Y, X), 7)
; and( add( and(X, 24), Y), 24)
; and( add( and(X, -32) ,Y) ,-32)
