#include <stdio.h>

int a() {
  return 1+1;
}

int fez() {
  return a()+1;
}

int main() {
  fez();
  return 0;
}
