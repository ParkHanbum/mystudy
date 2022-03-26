#include <stdio.h>
void foo();

void (*t)() = foo;
void (*tt)() = (void (*)())((long)&foo + 1);

long test = ((long)&foo) + 65536;
long test2 = (long)&foo + 1;


void foo()
{
    puts("TEST\n");
}


int main()
{
    ((int (*)()) (test - 65536))();
    ((int (*)()) (test2-1))();

    return 0;
}
