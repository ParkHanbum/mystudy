//=============================================================================
// FILE:
//      input_for_cc.c
//
// DESCRIPTION:
//      Sample input file for CallCounter analysis.
//
// License: MIT
//=============================================================================
void foo() { }
void bar() {foo(); }
void fez() {bar(); }

int main() {
  foo();
  bar();
  fez();

  void (*t)();
  t = foo;
  t(1);
  t = bar;
  t();
  t = fez;
  t();

  return 0;
}
