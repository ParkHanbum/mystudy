//=============================================================================
// FILE:
//      input_for_cc.c
//
// DESCRIPTION:
//      Sample input file for CallCounter analysis.
//
// License: MIT
//=============================================================================

/*
void foo() {
	puts("called foo\n");
}
void bar() {
	puts("called bar\n");
	foo();
}
void fez() {
	puts("called fez\n");
	bar();
}
*/

void fez() {
	puts("called fez\n");
}

int main() {
  fez();
  return 0;
}
